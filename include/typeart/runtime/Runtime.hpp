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

#ifndef TYPEART_RUNTIME_H
#define TYPEART_RUNTIME_H

#include "../db/Database.hpp"
#include "AccessCounter.hpp"
#include "Runtime.h"

#include <cstddef>
#include <string>

namespace typeart::runtime {

struct PointerInfo final {
  const void* base_addr = nullptr;
  type_id_t type_id     = type_id_t::invalid;
  size_t count          = 0;
  const void* debug     = nullptr;
};

std::optional<PointerInfo> getPointerInfo(const void* addr);
const AllocationInfo* getAllocationInfo(alloc_id_t alloc_id);

struct ScopeGuard final {
  ScopeGuard();
  ~ScopeGuard();

  ScopeGuard(const ScopeGuard&) = delete;
  ScopeGuard(ScopeGuard&&)      = delete;

  ScopeGuard& operator=(const ScopeGuard&) = delete;
  ScopeGuard& operator=(ScopeGuard&&) = delete;

  bool shouldTrack() const;
};

Recorder& getRecorder();
ScopeGuard scopeGuard();

std::string toString(const void* addr, type_id_t type_id, size_t count, size_t typeSize, const void* calledFrom);
std::string toString(const void* addr, type_id_t type_id, size_t count, const void* calledFrom);
std::string toString(const void* addr, const PointerInfo& pointer_info);

}  // namespace typeart::runtime

#endif  // TYPEART_RUNTIME_H
