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

#ifndef TYPEART_TYPEDATABASE_H
#define TYPEART_TYPEDATABASE_H

#include <memory>
#include <optional>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace typeart {

enum class StructTypeFlag : int { USER_DEFINED = 1, LLVM_VECTOR = 2 };

struct StructTypeInfo {
  int type_id;
  std::string name;
  size_t extent;
  size_t num_members;
  std::vector<size_t> offsets;
  std::vector<int> member_types;
  std::vector<size_t> array_sizes;
  StructTypeFlag flag;
};

using allocation_id_t = int;

struct AllocationInfo {
  allocation_id_t allocation_id;
  int type_id;

  std::optional<size_t> count;

  // Offset from the pointer to the memory used by this allocation to the
  // pointer returned to the user.
  // Note: for heap allocations, the actual value is ignored and only
  // used as a hint, that the allocation has a non-standard offset.
  // As the actual offset may not be statically known it is stored
  // within the actual allocation.
  std::optional<ptrdiff_t> base_ptr_offset;
};

class TypeDatabase {
 public:
  virtual void registerStruct(const StructTypeInfo& struct_info) = 0;

  virtual allocation_id_t getOrCreateAllocationId(int type_id, std::optional<size_t> count,
                                                  std::optional<ptrdiff_t> base_ptr_offset) = 0;

  virtual void registerAllocations(std::vector<AllocationInfo> allocation_info) = 0;

  [[nodiscard]] virtual bool isUnknown(int type_id) const = 0;

  [[nodiscard]] virtual bool isValid(int type_id) const = 0;

  [[nodiscard]] virtual bool isReservedType(int type_id) const = 0;

  [[nodiscard]] virtual bool isBuiltinType(int type_id) const = 0;

  [[nodiscard]] virtual bool isStructType(int type_id) const = 0;

  [[nodiscard]] virtual bool isUserDefinedType(int type_id) const = 0;

  [[nodiscard]] virtual bool isVectorType(int type_id) const = 0;

  [[nodiscard]] virtual const std::string& getTypeName(int type_id) const = 0;

  [[nodiscard]] virtual const StructTypeInfo* getStructInfo(int type_id) const = 0;

  [[nodiscard]] virtual const AllocationInfo* getAllocationInfo(allocation_id_t allocation_id) const = 0;

  [[nodiscard]] virtual size_t getTypeSize(int type_id) const = 0;

  [[nodiscard]] virtual const std::vector<StructTypeInfo>& getStructList() const = 0;

  [[nodiscard]] virtual const std::vector<AllocationInfo>& getAllocationInfo() const = 0;

  virtual ~TypeDatabase() = default;
};

std::pair<std::unique_ptr<TypeDatabase>, std::error_code> make_database(const std::string& file);

}  // namespace typeart

#endif  // TYPEART_TYPEDATABASE_H
