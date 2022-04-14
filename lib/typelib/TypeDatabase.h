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

#include "TypeInterface.h"

#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace typeart {

struct type_id_t {
  using value_type = int;

  static const type_id_t unknown_type;

 private:
  value_type _value = TYPEART_UNKNOWN_TYPE;

 public:
  type_id_t() = default;

  type_id_t(value_type value) : _value(value) {
  }

  template <class T>
  type_id_t(T t) = delete;

  value_type value() const {
    return _value;
  }
};

inline bool operator==(const type_id_t& lhs, const type_id_t& rhs) {
  return lhs.value() == rhs.value();
}

inline bool operator!=(const type_id_t& lhs, const type_id_t& rhs) {
  return !(lhs == rhs);
}

inline bool operator<(const type_id_t& lhs, const type_id_t& rhs) {
  return lhs.value() < rhs.value();
}

std::ostream& operator<<(std::ostream& os, const type_id_t& alloc_id);

struct alloc_id_t {
  using value_type = int;

  static const alloc_id_t invalid;

 private:
  value_type _value = 0;

 public:
  alloc_id_t() = default;

  alloc_id_t(value_type value) : _value(value) {
  }

  template <class T>
  alloc_id_t(T t) = delete;

  value_type value() const {
    return _value;
  }
};

inline bool operator==(const alloc_id_t& lhs, const alloc_id_t& rhs) {
  return lhs.value() == rhs.value();
}

inline bool operator!=(const alloc_id_t& lhs, const alloc_id_t& rhs) {
  return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const alloc_id_t& alloc_id);

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

class TypeDatabase {
 public:
  virtual void registerStruct(const StructTypeInfo& struct_info) = 0;

  virtual alloc_id_t getOrCreateAllocationId(type_id_t type_id, std::optional<size_t> count,
                                             std::optional<ptrdiff_t> base_ptr_offset) = 0;

  virtual void registerAllocations(std::vector<AllocationInfo> allocation_info) = 0;

  [[nodiscard]] virtual bool isUnknown(type_id_t type_id) const = 0;

  [[nodiscard]] virtual bool isValid(type_id_t type_id) const = 0;

  [[nodiscard]] virtual bool isValid(alloc_id_t alloc_id) const = 0;

  [[nodiscard]] virtual bool isReservedType(type_id_t type_id) const = 0;

  [[nodiscard]] virtual bool isBuiltinType(type_id_t type_id) const = 0;

  [[nodiscard]] virtual bool isStructType(type_id_t type_id) const = 0;

  [[nodiscard]] virtual bool isUserDefinedType(type_id_t type_id) const = 0;

  [[nodiscard]] virtual bool isVectorType(type_id_t type_id) const = 0;

  [[nodiscard]] virtual const std::string& getTypeName(type_id_t type_id) const = 0;

  [[nodiscard]] virtual const StructTypeInfo* getStructInfo(type_id_t type_id) const = 0;

  [[nodiscard]] virtual const AllocationInfo* getAllocationInfo(alloc_id_t alloc_id) const = 0;

  [[nodiscard]] virtual size_t getTypeSize(type_id_t type_id) const = 0;

  [[nodiscard]] virtual const std::vector<StructTypeInfo>& getStructList() const = 0;

  [[nodiscard]] virtual const std::vector<AllocationInfo>& getAllocationInfo() const = 0;

  virtual ~TypeDatabase() = default;
};

std::pair<std::unique_ptr<TypeDatabase>, std::error_code> make_database(const std::string& file);

}  // namespace typeart

namespace std {
template <>
struct hash<typeart::type_id_t> {
  std::size_t operator()(const typeart::type_id_t& type_id) const {
    return hash<typeart::type_id_t::value_type>{}(type_id.value());
  }
};

}  // namespace std

#endif  // TYPEART_TYPEDATABASE_H
