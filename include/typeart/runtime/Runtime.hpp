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
  // The base address of the whole allocation.
  const void* base_addr = nullptr;

  // The type id of of the allocation or the allocation's element type.
  type_id_t type_id = type_id_t::invalid;

  // The element count of the allocation w.r.t. the address this instance was
  // queried with. (e.g. the element count of a subarray)
  size_t count = 0;

  // The return address recorded on allocation, if available.
  const void* debug = nullptr;
};

std::ostream& operator<<(std::ostream& os, const PointerInfo& pointer_info);

typeart_status getPointerInfo(const void* addr, PointerInfo& result);
typeart_status getContainingInfo(const void* addr, PointerInfo& result, size_t* offset);

typeart_status getStructType(const void* addr, const StructType** structInfo);
typeart_status getStructType(type_id_t type_id, const StructType** structInfo);
typeart_status getSubTypeInfo(const void* baseAddr, size_t offset, const StructType& outer_type,
                              PointerInfo& subtype_pointer_info, size_t* subtype_offset);

const std::string& getTypeName(type_id_t type_id);
size_t getTypeSize(type_id_t type_id);
bool isUnknown(type_id_t type_id);
bool isValid(type_id_t type_id);
bool isReservedType(type_id_t type_id);
bool isBuiltinType(type_id_t type_id);
bool isStructType(type_id_t type_id);
bool isUserDefinedType(type_id_t type_id);
bool isVectorType(type_id_t type_id);

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
const AllocationInfo* getAllocationInfo(alloc_id_t alloc_id);

}  // namespace typeart::runtime

#endif  // TYPEART_RUNTIME_H
