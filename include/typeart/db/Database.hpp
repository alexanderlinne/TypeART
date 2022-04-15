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

#include "Types.hpp"

#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

namespace typeart {

enum class StructTypeFlag : int { USER_DEFINED = 1, LLVM_VECTOR = 2 };

struct StructTypeInfo {
  type_id_t type_id;
  std::string name;
  size_t extent;
  size_t num_members;
  std::vector<size_t> offsets;
  std::vector<type_id_t> member_types;
  std::vector<size_t> array_sizes;
  StructTypeFlag flag;
};

struct AllocationInfo {
  alloc_id_t alloc_id = alloc_id_t::invalid;
  type_id_t type_id   = type_id_t::unknown_type;

  std::optional<size_t> count;

  // Offset from the pointer to the memory used by this allocation to the
  // pointer returned to the user.
  // Note: for heap allocations, the actual value is ignored and only
  // used as a hint, that the allocation has a non-standard offset.
  // As the actual offset may not be statically known it is stored
  // within the actual allocation.
  std::optional<ptrdiff_t> base_ptr_offset;
};

class Database {
 public:
  [[nodiscard]] static std::optional<Database> load(const std::string& file);
  [[nodiscard]] bool store(const std::string& file) const;

 public:
  void registerStruct(const StructTypeInfo& struct_info);

  alloc_id_t getOrCreateAllocationId(type_id_t type_id, std::optional<size_t> count,
                                     std::optional<ptrdiff_t> base_ptr_offset);

  void registerAllocations(std::vector<AllocationInfo> allocation_info);

  [[nodiscard]] bool isUnknown(type_id_t type_id) const;

  [[nodiscard]] bool isValid(type_id_t type_id) const;

  [[nodiscard]] bool isValid(alloc_id_t alloc_id) const;

  [[nodiscard]] bool isReservedType(type_id_t type_id) const;

  [[nodiscard]] bool isBuiltinType(type_id_t type_id) const;

  [[nodiscard]] bool isStructType(type_id_t type_id) const;

  [[nodiscard]] bool isUserDefinedType(type_id_t type_id) const;

  [[nodiscard]] bool isVectorType(type_id_t type_id) const;

  [[nodiscard]] const std::string& getTypeName(type_id_t type_id) const;

  [[nodiscard]] const StructTypeInfo* getStructInfo(type_id_t type_id) const;
  [[nodiscard]] const std::vector<StructTypeInfo>& getStructInfo() const;

  [[nodiscard]] const AllocationInfo* getAllocationInfo(alloc_id_t alloc_id) const;

  [[nodiscard]] size_t getTypeSize(type_id_t type_id) const;

  static const std::array<std::string, 11> BuiltinNames;
  static const std::array<size_t, 11> BuiltinSizes;
  static const std::string UnknownStructName;

 private:
  std::vector<AllocationInfo> allocation_info;
  std::vector<StructTypeInfo> struct_info_vec;
  std::unordered_map<type_id_t, int> typeid_to_list_index;
};

}  // namespace typeart
