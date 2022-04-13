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

#ifndef TYPEART_TYPERESOLUTION_H
#define TYPEART_TYPERESOLUTION_H

#include "AccessCounter.h"
#include "RuntimeData.h"
#include "RuntimeInterface.h"
#include "TypeDB.h"
#include "TypeInterface.h"

#include <cstddef>
#include <string>

namespace typeart::runtime {

using BuiltinType = typeart_builtin_type;

struct PointerInfo;

class TypeResolution {
  const TypeDB& typeDB;

 public:
  using TypeArtStatus = typeart_status;

  TypeResolution(const TypeDB& db);

  const std::string& getTypeName(int type_id) const;
  size_t getTypeSize(int type_id) const;

  [[nodiscard]] size_t getMemberIndex(typeart_struct_layout structInfo, size_t offset) const;

  TypeArtStatus getSubTypeInfo(const void* baseAddr, size_t offset, typeart_struct_layout containerInfo, int* subType,
                               const void** subTypeBaseAddr, size_t* subTypeOffset, size_t* subTypeCount) const;

  TypeArtStatus getSubTypeInfo(const void* baseAddr, size_t offset, const StructTypeInfo& containerInfo, int* subType,
                               const void** subTypeBaseAddr, size_t* subTypeOffset, size_t* subTypeCount) const;

  TypeArtStatus getTypeInfoInternal(const void* baseAddr, size_t offset, const StructTypeInfo& containerInfo, int* type,
                                    size_t* count) const;

  TypeArtStatus getTypeInfo(const void* addr, const void* basePtr, const PointerInfo& ptrInfo, int* type,
                            size_t* count) const;

  TypeArtStatus getContainingTypeInfo(const void* addr, const void* basePtr, const PointerInfo& ptrInfo, size_t* count,
                                      size_t* offset) const;

  TypeArtStatus getBuiltinInfo(const void* addr, const PointerInfo& ptrInfo, BuiltinType* type) const;

  TypeArtStatus getStructInfo(int type_id, const StructTypeInfo** structInfo) const;

  TypeArtStatus getAllocationInfo(allocation_id_t allocation_id, const AllocationInfo** allocation_info) const;

  std::string toString(const void* memAddr, int typeId, size_t count, size_t typeSize, const void* calledFrom) const;
  std::string toString(const void* memAddr, int typeId, size_t count, const void* calledFrom) const;
  std::string toString(const void* addr, const PointerInfo& info) const;

  bool isUnknown(int type_id) const;
  bool isValid(int type_id) const;
  bool isReservedType(int type_id) const;
  bool isBuiltinType(int type_id) const;
  bool isStructType(int type_id) const;
  bool isUserDefinedType(int type_id) const;
  bool isVectorType(int type_id) const;
};

}  // namespace typeart::runtime

#endif  // TYPEART_TYPERESOLUTION_H
