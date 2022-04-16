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

#pragma once

#include "db/Database.hpp"
#include "runtime/Runtime.hpp"

#include <cstddef>
#include <string>

namespace typeart::runtime {

using BuiltinType = typeart_builtin_type;

class TypeResolution {
  const Database& db;

 public:
  using TypeArtStatus = typeart_status;

  TypeResolution(const Database& db);

  const std::string& getTypeName(type_id_t type_id) const;
  size_t getTypeSize(type_id_t type_id) const;

  [[nodiscard]] size_t getMemberIndex(typeart_struct_layout structInfo, size_t offset) const;

  TypeArtStatus getSubTypeInfo(const void* baseAddr, size_t offset, typeart_struct_layout containerInfo,
                               type_id_value* subType, const void** subTypeBaseAddr, size_t* subTypeOffset,
                               size_t* subTypeCount) const;

  TypeArtStatus getSubTypeInfo(const void* baseAddr, size_t offset, const StructType& containerInfo,
                               type_id_value* subType, const void** subTypeBaseAddr, size_t* subTypeOffset,
                               size_t* subTypeCount) const;

  TypeArtStatus getTypeInfoInternal(const void* baseAddr, size_t offset, const StructType& containerInfo,
                                    type_id_value* type, size_t* count) const;

  TypeArtStatus getTypeInfo(const void* addr, const PointerInfo& ptrInfo, type_id_value* type, size_t* count) const;

  TypeArtStatus getContainingTypeInfo(const void* addr, const PointerInfo& ptrInfo, size_t* count,
                                      size_t* offset) const;

  TypeArtStatus getBuiltinInfo(const void* addr, const PointerInfo& ptrInfo, BuiltinType* type) const;

  TypeArtStatus getStructType(type_id_t type_id, const StructType** structInfo) const;

  TypeArtStatus getAllocationInfo(alloc_id_t alloc_id, const AllocationInfo** allocation_info) const;

  std::string toString(const void* addr, type_id_t type_id, size_t count, size_t typeSize,
                       const void* calledFrom) const;
  std::string toString(const void* addr, type_id_t type_id, size_t count, const void* calledFrom) const;
  std::string toString(const void* addr, const PointerInfo& pointer_info) const;

  bool isUnknown(type_id_t type_id) const;
  bool isValid(type_id_t type_id) const;
  bool isReservedType(type_id_t type_id) const;
  bool isBuiltinType(type_id_t type_id) const;
  bool isStructType(type_id_t type_id) const;
  bool isUserDefinedType(type_id_t type_id) const;
  bool isVectorType(type_id_t type_id) const;
};

}  // namespace typeart::runtime
