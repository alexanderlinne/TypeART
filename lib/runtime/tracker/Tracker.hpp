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

#ifndef TYPEART_ALLOCATIONTRACKING_H
#define TYPEART_ALLOCATIONTRACKING_H

#include "AllocMapWrapper.hpp"
#include "db/Database.hpp"
#include "runtime/AccessCounter.hpp"

#include <cstddef>

namespace llvm {
template <typename T>
class Optional;
}  // namespace llvm

namespace typeart::runtime::tracker {

enum class AllocState : unsigned {
  NO_INIT          = 1 << 0,
  OK               = 1 << 1,
  ADDR_SKIPPED     = 1 << 2,
  NULL_PTR         = 1 << 3,
  ZERO_COUNT       = 1 << 4,
  NULL_ZERO        = 1 << 5,
  ADDR_REUSE       = 1 << 6,
  UNKNOWN_TYPE_ID  = 1 << 7,
  UNKNOWN_ALLOC_ID = 1 << 8
};

inline unsigned operator&(const AllocState& lhs, const AllocState& rhs) {
  return static_cast<unsigned>(lhs) & static_cast<unsigned>(rhs);
}

enum class FreeState : unsigned {
  NO_INIT      = 1 << 0,
  OK           = 1 << 1,
  ADDR_SKIPPED = 1 << 2,
  NULL_PTR     = 1 << 3,
  UNREG_ADDR   = 1 << 4,
};

class Tracker {
  PointerMap wrapper;

 public:
  static Tracker& get() {
    static Tracker instance;
    return instance;
  }

 private:
  Tracker();

 public:
  void onAlloc(const void* addr, alloc_id_t alloc_id, size_t count, const void* retAddr);
  void onAlloc(const void* addr, type_id_t type_id, size_t count, const void* retAddr);

  void onAllocStack(const void* addr, alloc_id_t alloc_id, size_t count, const void* retAddr);

  void onAllocGlobal(const void* addr, alloc_id_t alloc_id, size_t count, const void* retAddr);

  void onFreeHeap(const void* addr, const void* retAddr);

  void onLeaveScope(int alloca_count, const void* retAddr);

  std::optional<PointerInfo> getPointerInfo(const void* addr);

 private:
  AllocState doAlloc(const void* addr, alloc_id_t alloc_id, size_t count, const void* retAddr);
  AllocState doAlloc(const void* addr, type_id_t type_id, size_t count, const void* retAddr);

  FreeState doFreeHeap(const void* addr, const void* retAddr);
};

}  // namespace typeart::runtime::tracker

#endif  // TYPEART_ALLOCATIONTRACKING_H
