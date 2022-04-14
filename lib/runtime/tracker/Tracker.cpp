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

#include "Tracker.h"

#include "CallbackInterface.h"
#include "TypeDB.h"
#include "runtime/AccessCounter.h"
#include "runtime/Runtime.h"
#include "runtime/TypeResolution.h"
#include "support/Logger.h"

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

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define CONCAT_(x, y) x##y
#define CONCAT(x, y) CONCAT_(x, y)
#define GUARDNAME CONCAT(typeart_guard_, __LINE__)

namespace typeart::runtime::tracker {

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

Tracker::Tracker(const TypeDB& db, Recorder& recorder) : typeDB{db}, recorder{recorder} {
}

void Tracker::onAlloc(const void* addr, alloc_id_t alloc_id, size_t count, const void* retAddr) {
  const auto status = doAlloc(addr, alloc_id, count, retAddr);
  if (!(status & AllocState::ADDR_SKIPPED)) {
    recorder.incHeapAlloc(typeDB.getAllocationInfo(alloc_id)->type_id, count);
  }
  if (!(status & AllocState::UNKNOWN_ALLOC_ID)) {
    LOG_TRACE("Alloc " << Runtime::toString(addr, alloc_id, typeDB.getAllocationInfo(alloc_id)->type_id, count, retAddr)
                       << " " << 'H');
  }
}

void Tracker::onAllocStack(const void* addr, alloc_id_t alloc_id, size_t count, const void* retAddr) {
  const auto status = doAlloc(addr, alloc_id, count, retAddr);
  if (!(status & AllocState::ADDR_SKIPPED)) {
    threadData.stackVars.push_back(addr);
    recorder.incStackAlloc(typeDB.getAllocationInfo(alloc_id)->type_id, count);
  }
  if (!(status & AllocState::UNKNOWN_ALLOC_ID)) {
    LOG_TRACE("Alloc " << Runtime::toString(addr, alloc_id, typeDB.getAllocationInfo(alloc_id)->type_id, count, retAddr)
                       << " " << 'S');
  }
}

void Tracker::onAllocGlobal(const void* addr, alloc_id_t alloc_id, size_t count, const void* retAddr) {
  const auto status = doAlloc(addr, alloc_id, count, retAddr);
  if (!(status & AllocState::ADDR_SKIPPED)) {
    recorder.incGlobalAlloc(typeDB.getAllocationInfo(alloc_id)->type_id, count);
  }
  if (!(status & AllocState::UNKNOWN_ALLOC_ID)) {
    LOG_TRACE("Alloc " << Runtime::toString(addr, alloc_id, typeDB.getAllocationInfo(alloc_id)->type_id, count, retAddr)
                       << " " << 'G');
  }
}

AllocState Tracker::doAlloc(const void* addr, alloc_id_t alloc_id, size_t count, const void* retAddr) {
  AllocState status = AllocState::NO_INIT;
  if (unlikely(!typeDB.isValid(alloc_id))) {
    status |= AllocState::UNKNOWN_ALLOC_ID | AllocState::ADDR_SKIPPED;
    LOG_ERROR("Allocation with unknown alloc_id! Skipping...");
    return status;
  }
  auto type_id = typeDB.getAllocationInfo(alloc_id)->type_id;

  // Calling malloc with size 0 may return a nullptr or some address that can not be written to.
  // In the second case, the allocation is tracked anyway so that onFree() does not report an error.
  // On the other hand, an allocation on address 0x0 with size > 0 is an actual error.
  if (unlikely(count == 0)) {
    recorder.incZeroLengthAddr();
    status |= AllocState::ZERO_COUNT;
    LOG_WARNING("Zero-size allocation " << Runtime::toString(addr, alloc_id, type_id, count, retAddr));
    if (addr == nullptr) {
      recorder.incZeroLengthAndNullAddr();
      LOG_ERROR("Zero-size and nullptr allocation " << Runtime::toString(addr, alloc_id, type_id, count, retAddr));
      return status | AllocState::NULL_ZERO | AllocState::ADDR_SKIPPED;
    }
  } else if (unlikely(addr == nullptr)) {
    recorder.incNullAddr();
    LOG_ERROR("Nullptr allocation " << Runtime::toString(addr, alloc_id, type_id, count, retAddr));
    return status | AllocState::NULL_PTR | AllocState::ADDR_SKIPPED;
  }

  const auto overridden = wrapper.put(addr, PointerInfo{addr, alloc_id, count, retAddr});

  if (unlikely(overridden)) {
    recorder.incAddrReuse();
    status |= AllocState::ADDR_REUSE;
    LOG_WARNING("Pointer already in map " << Runtime::toString(addr, alloc_id, type_id, count, retAddr));
    // LOG_WARNING("Overridden data in map " << toString(addr, def));
  }

  return status | AllocState::OK;
}

FreeState Tracker::doFreeHeap(const void* addr, const void* retAddr) {
  if (unlikely(addr == nullptr)) {
    LOG_ERROR("Free on nullptr "
              << "(" << retAddr << ")");
    return FreeState::ADDR_SKIPPED | FreeState::NULL_PTR;
  }
  llvm::Optional<PointerInfo> removed = wrapper.remove(addr);

  if (unlikely(!removed)) {
    LOG_ERROR("Free on unregistered address " << addr << " (" << retAddr << ")");
    return FreeState::ADDR_SKIPPED | FreeState::UNREG_ADDR;
  }

  auto type_id = typeDB.getAllocationInfo(removed->alloc_id)->type_id;
  LOG_TRACE("Free " << Runtime::get().toString(addr, removed->alloc_id, type_id, removed->count, removed->debug));

  if constexpr (!std::is_same_v<Recorder, softcounter::NoneRecorder>) {
    recorder.incHeapFree(type_id, removed->count);
  }
  return FreeState::OK;
}

void Tracker::onFreeHeap(const void* addr, const void* retAddr) {
  const auto status = doFreeHeap(addr, retAddr);
  if (FreeState::OK == status) {
    recorder.decHeapAlloc();
  }
}

void Tracker::onLeaveScope(int alloca_count, const void* retAddr) {
  if (unlikely(alloca_count > static_cast<int>(threadData.stackVars.size()))) {
    LOG_ERROR("Stack is smaller than requested de-allocation count. alloca_count: " << alloca_count << ". size: "
                                                                                    << threadData.stackVars.size());
    alloca_count = threadData.stackVars.size();
  }

  const auto cend      = threadData.stackVars.cend();
  const auto start_pos = (cend - alloca_count);
  LOG_TRACE("Freeing stack (" << alloca_count << ")  " << std::distance(start_pos, threadData.stackVars.cend()))

  wrapper.remove_range(start_pos, cend, [&](llvm::Optional<PointerInfo>& removed, const void* addr) {
    if (unlikely(!removed)) {
      LOG_ERROR("Free on unregistered address " << addr << " (" << retAddr << ")");
    } else {
      auto type_id = typeDB.getAllocationInfo(removed->alloc_id)->type_id;
      LOG_TRACE("Free " << Runtime::get().toString(addr, removed->alloc_id, type_id, removed->count, removed->debug));
      if constexpr (!std::is_same_v<Recorder, softcounter::NoneRecorder>) {
        recorder.incStackFree(typeDB.getAllocationInfo(removed->alloc_id)->type_id, removed->count);
      }
    }
  });

  threadData.stackVars.erase(start_pos, cend);
  recorder.decStackAlloc(alloca_count);
  LOG_TRACE("Stack after free: " << threadData.stackVars.size());
}
// Base address
std::optional<PointerInfo> Tracker::getPointerInfo(const void* addr) {
  auto result = wrapper.find(addr);
  if (result.hasValue()) {
    return result->second;
  }
  return {};
}

}  // namespace typeart::runtime::tracker
