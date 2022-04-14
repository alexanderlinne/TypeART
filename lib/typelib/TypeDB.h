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

#ifndef LLVM_MUST_SUPPORT_TYPECONFIG_H
#define LLVM_MUST_SUPPORT_TYPECONFIG_H

#include "TypeDatabase.h"

#include <array>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace typeart {

class TypeDB final : public TypeDatabase {
 public:
  void clear();

  void registerStruct(const StructTypeInfo& struct_type) override;

  alloc_id_t getOrCreateAllocationId(type_id_t type_id, std::optional<size_t> count,
                                     std::optional<ptrdiff_t> base_ptr_offset) override;

  void registerAllocations(std::vector<AllocationInfo> allocations) override;

  bool isUnknown(type_id_t type_id) const override;

  bool isValid(type_id_t type_id) const override;

  bool isValid(alloc_id_t alloc_id) const override;

  bool isReservedType(type_id_t type_id) const override;

  bool isBuiltinType(type_id_t type_id) const override;

  bool isStructType(type_id_t type_id) const override;

  bool isUserDefinedType(type_id_t type_id) const override;

  bool isVectorType(type_id_t type_id) const override;

  const std::string& getTypeName(type_id_t type_id) const override;

  const StructTypeInfo* getStructInfo(type_id_t type_id) const override;

  const AllocationInfo* getAllocationInfo(alloc_id_t alloc_id) const override;

  size_t getTypeSize(type_id_t type_id) const override;

  const std::vector<StructTypeInfo>& getStructList() const override;

  const std::vector<AllocationInfo>& getAllocationInfo() const override;

  static const std::array<std::string, 11> BuiltinNames;
  static const std::array<size_t, 11> BuiltinSizes;
  static const std::string UnknownStructName;

 private:
  std::vector<AllocationInfo> allocation_info;
  std::vector<StructTypeInfo> struct_info_vec;
  std::unordered_map<type_id_t, int> typeid_to_list_index;
};

}  // namespace typeart

#endif  // LLVM_MUST_SUPPORT_TYPECONFIG_H
