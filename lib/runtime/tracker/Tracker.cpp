// TypeART library
//
// Copyright (c) 2017-2022 TypeART Authors
// Distributed under the BSD 3-Clause license.
// (See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/BSD-3-Clause)
//
// Project home: https://github.com/tudasc/TypeART
//
// SPDX-License-Identifier: BSD-3-Clause
//

#include "runtime/tracker/Tracker.hpp"

#include "CallbackInterface.h"
#include "db/Database.hpp"
#include "runtime/AccessCounter.hpp"
#include "runtime/Runtime.hpp"
#include "support/Logger.hpp"

#include "llvm/ADT/Optional.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <type_traits>
#include <vector>

#ifdef TYPEART_BTREE
using namespace btree;
#endif

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define CONCAT_(x, y) x##y
#define CONCAT(x, y)  CONCAT_(x, y)
#define GUARDNAME     CONCAT(typeart_guard_, __LINE__)

namespace typeart::tracker {

namespace detail {
template <class...>
constexpr std::false_type always_false{};
}  // namespace detail

template <typename Enum>
inline Enum operator|(Enum lhs, Enum rhs) {
  if constexpr (std::is_enum_v<Enum> && (std::is_same_v<Enum, AllocState> || std::is_same_v<Enum, FreeState>)) {
    using enum_t = typename std::underlying_type<Enum>::type;
    return static_cast<Enum>(static_cast<enum_t>(lhs) | static_cast<enum_t>(rhs));
  } else {
    static_assert(detail::always_false<Enum>);
  }
}
template <typename Enum>
inline void operator|=(Enum& lhs, Enum rhs) {
  if constexpr (std::is_enum_v<Enum> && (std::is_same_v<Enum, AllocState> || std::is_same_v<Enum, FreeState>)) {
    lhs = lhs | rhs;
  } else {
    static_assert(detail::always_false<Enum>);
  }
}

template <typename Enum>
inline Enum operator&(Enum lhs, Enum rhs) {
  if constexpr (std::is_enum_v<Enum> && std::is_same_v<Enum, AllocState>) {
    using enum_t = typename std::underlying_type<Enum>::type;
    return static_cast<Enum>(static_cast<enum_t>(lhs) & static_cast<enum_t>(rhs));
  } else {
    static_assert(detail::always_false<Enum>);
  }
}

template <typename Enum>
inline typename std::underlying_type<Enum>::type operator==(Enum lhs, Enum rhs) {
  if constexpr (std::is_enum_v<Enum> && std::is_same_v<Enum, AllocState>) {
    using enum_t = typename std::underlying_type<Enum>::type;
    return static_cast<enum_t>(lhs) & static_cast<enum_t>(rhs);
  } else {
    static_assert(detail::always_false<Enum>);
  }
}

namespace {
struct ThreadData final {
  RuntimeT::Stack stackVars;

  ThreadData() {
    stackVars.reserve(RuntimeT::StackReserve);
  }
};

thread_local ThreadData threadData;

}  // namespace

Tracker::Tracker() {
}

void Tracker::onAlloc(const void* addr, meta_id_t meta_id, size_t count, const void* retAddr) {
  const auto status = doAlloc(addr, meta_id, count, retAddr);
  if (!(status & AllocState::UNKNOWN_META_ID)) {
    const auto meta  = getDatabase().getMeta(meta_id);
    const auto alloc = meta::dyn_cast<meta::HeapAllocation>(meta);
    if (unlikely(alloc == nullptr)) {
      LOG_ERROR("Unexpected meta type. Expected HeapAllocation, but found {}", meta->get_kind());
      return;
    }
    if (!(status & AllocState::ADDR_SKIPPED)) {
      getRecorder().incHeapAlloc(alloc, count);
    }
    auto pointer_info = PointerInfo{pointer{addr}, *alloc, alloc->get_type(), count};
    LOG_TRACE("Alloc heap {}", pointer_info);
  }
}

void Tracker::onAllocStack(const void* addr, meta_id_t meta_id, size_t count, const void* retAddr) {
  const auto status = doAlloc(addr, meta_id, count, retAddr);
  if (!(status & AllocState::UNKNOWN_META_ID)) {
    const auto meta  = getDatabase().getMeta(meta_id);
    const auto alloc = meta::dyn_cast<meta::StackAllocation>(meta);
    if (unlikely(alloc == nullptr)) {
      LOG_ERROR("Unexpected meta type. Expected StackAllocation, but found {}", meta->get_kind());
      return;
    }
    if (!(status & AllocState::ADDR_SKIPPED)) {
      threadData.stackVars.push_back(addr);
      getRecorder().incStackAlloc(alloc, count);
    }
    auto pointer_info = PointerInfo{pointer{addr}, *alloc, alloc->get_type(), count};
    LOG_TRACE("Alloc stack {}", pointer_info);
  }
}

void Tracker::onAllocGlobal(const void* addr, meta_id_t meta_id, size_t count, const void* retAddr) {
  const auto status = doAlloc(addr, meta_id, count, retAddr);
  if (!(status & AllocState::UNKNOWN_META_ID)) {
    const auto meta  = getDatabase().getMeta(meta_id);
    const auto alloc = meta::dyn_cast<meta::GlobalAllocation>(meta);
    if (unlikely(alloc == nullptr)) {
      LOG_ERROR("Unexpected meta type. Expected GlobalAllocation, but found {}", meta->get_kind());
      return;
    }
    if (!(status & AllocState::ADDR_SKIPPED)) {
      getRecorder().incGlobalAlloc(alloc, count);
    }
    auto pointer_info = PointerInfo{pointer{addr}, *alloc, alloc->get_type(), count};
    LOG_TRACE("Alloc global {}", pointer_info);
  }
}

AllocState Tracker::doAlloc(const void* addr, meta_id_t meta_id, size_t count, const void* retAddr) {
  const auto meta = getDatabase().getMeta(meta_id);
  if (unlikely(meta == nullptr)) {
    LOG_ERROR("Allocation with unknown meta_id! Skipping...");
    return AllocState::UNKNOWN_META_ID | AllocState::ADDR_SKIPPED;
  }
  const auto alloc = meta::dyn_cast<meta::Allocation>(meta);

  AllocState status = AllocState::NO_INIT;
  auto& recorder    = getRecorder();

  // Calling malloc with size 0 may return a nullptr or some address that can not be written to.
  // In the second case, the allocation is tracked anyway so that onFree() does not report an error.
  // On the other hand, an allocation on address 0x0 with size > 0 is an actual error.
  const auto pointer_info = PointerInfo{pointer{addr}, *alloc, alloc->get_type(), count};
  if (unlikely(count == 0)) {
    recorder.incZeroLengthAddr();
    status |= AllocState::ZERO_COUNT;
    LOG_WARNING("Zero-size allocation {}", pointer_info);
    if (addr == nullptr) {
      recorder.incZeroLengthAndNullAddr();
      LOG_ERROR("Zero-size and nullptr allocation {}", pointer_info);
      return status | AllocState::NULL_ZERO | AllocState::ADDR_SKIPPED;
    }
  } else if (unlikely(addr == nullptr)) {
    recorder.incNullAddr();
    LOG_ERROR("Nullptr allocation {}", pointer_info);
    return status | AllocState::NULL_PTR | AllocState::ADDR_SKIPPED;
  }

  const auto overridden = wrapper.put(addr, pointer_info);
  if (unlikely(overridden)) {
    recorder.incAddrReuse();
    status |= AllocState::ADDR_REUSE;
    LOG_WARNING("Pointer already in map {}", pointer_info);
  }

  return status | AllocState::OK;
}

FreeState Tracker::doFreeHeap(const void* addr, const void* retAddr) {
  if (unlikely(addr == nullptr)) {
    LOG_TRACE("Free on nullptr ({})", retAddr);
    return FreeState::ADDR_SKIPPED | FreeState::NULL_PTR;
  }
  llvm::Optional<PointerInfo> removed = wrapper.remove(addr);

  if (unlikely(!removed)) {
    LOG_TRACE("Free on unregistered address {} ({})", addr, retAddr);
    return FreeState::ADDR_SKIPPED | FreeState::UNREG_ADDR;
  }

  LOG_TRACE("Free heap {}", *removed);

  if constexpr (!std::is_same_v<Recorder, softcounter::NoneRecorder>) {
    auto& meta = removed->getAllocation();
    auto alloc = meta::dyn_cast<meta::HeapAllocation>(&meta);
    if (unlikely(alloc == nullptr)) {
      LOG_ERROR("Unexpected meta type. Expected HeapAllocation, but found {}", meta.get_kind());
      return FreeState::ERROR;
    }
    getRecorder().incHeapFree(alloc, removed->getCount());
  }
  return FreeState::OK;
}

void Tracker::onFreeHeap(const void* addr, const void* retAddr) {
  const auto status = doFreeHeap(addr, retAddr);
  if (FreeState::OK == status) {
    getRecorder().decHeapAlloc();
  }
}

void Tracker::onLeaveScope(int alloca_count, const void* retAddr) {
  if (unlikely(alloca_count > static_cast<int>(threadData.stackVars.size()))) {
    LOG_ERROR("Stack is smaller than requested de-allocation count. alloca_count: {}. size: {}", alloca_count,
              threadData.stackVars.size());
    alloca_count = threadData.stackVars.size();
  }

  const auto cend      = threadData.stackVars.cend();
  const auto start_pos = (cend - alloca_count);
  LOG_TRACE("Freeing {} stack entries...", alloca_count);

  auto& recorder = getRecorder();
  wrapper.remove_range(start_pos, cend, [&](llvm::Optional<PointerInfo>& removed, const void* addr) {
    if (unlikely(!removed)) {
      LOG_TRACE("Free on unregistered address {} ({})", addr, retAddr);
    } else {
      LOG_TRACE("Free stack {}", *removed);
      if constexpr (!std::is_same_v<Recorder, softcounter::NoneRecorder>) {
        auto& meta = removed->getAllocation();
        auto alloc = meta::dyn_cast<meta::StackAllocation>(&meta);
        if (unlikely(alloc == nullptr)) {
          LOG_ERROR("Unexpected meta type. Expected StackAllocation, but found {}", meta.get_kind());
          return;
        }
        recorder.incStackFree(alloc, removed->getCount());
      }
    }
  });

  threadData.stackVars.erase(start_pos, cend);
  recorder.decStackAlloc(alloca_count);
  LOG_TRACE("{} remaining stack entries after free!", threadData.stackVars.size());
}
// Base address
std::optional<PointerInfo> Tracker::getPointerInfo(const void* addr) {
  auto result = wrapper.find(addr);
  if (result.hasValue()) {
    return result->second;
  }
  return {};
}

}  // namespace typeart::tracker
