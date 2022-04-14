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

#include "TypeDB.h"

#include "TypeIO.h"
#include "support/Logger.h"
#include "typelib/TypeInterface.h"

#include <iostream>
#include <utility>

namespace typeart {

std::pair<std::unique_ptr<TypeDatabase>, std::error_code> make_database(const std::string& file) {
  auto type_db = std::make_unique<TypeDB>();
  auto loaded  = io::load(type_db.get(), file);
  if (!loaded) {
    LOG_DEBUG("Database file not found: " << file)
  }
  return {std::move(type_db), loaded.getError()};
}

const std::array<std::string, 11> TypeDB::BuiltinNames = {
    "int8", "int16", "int32", "int64", "half", "float", "double", "float128", "x86_float80", "ppc_float128", "pointer"};

// TODO: Builtin ID changes lead tsto wrong type sizes/names
const std::array<size_t, 11> TypeDB::BuiltinSizes = {1,  2,
                                                     4,  8,
                                                     2,  4,
                                                     8,  16,
                                                     16,  // TODO: Always correct?
                                                     16, sizeof(void*)};

// TypeInfo TypeDB::InvalidType = TypeInfo{BUILTIN, TA_UNKNOWN_TYPE};

const std::string TypeDB::UnknownStructName{"typeart_unknown_struct"};

void TypeDB::clear() {
  struct_info_vec.clear();
  typeid_to_list_index.clear();
  // reverseTypeMap.clear();
}

bool TypeDB::isBuiltinType(type_id_t type_id) const {
  return type_id.value() >= TYPEART_INT8 && type_id.value() < TYPEART_NUM_VALID_IDS;
}

bool TypeDB::isReservedType(type_id_t type_id) const {
  return type_id.value() < TYPEART_NUM_RESERVED_IDS;
}

bool TypeDB::isStructType(type_id_t type_id) const {
  return type_id.value() >= TYPEART_NUM_RESERVED_IDS;
}

bool TypeDB::isUserDefinedType(type_id_t type_id) const {
  const auto* structInfo = getStructInfo(type_id);
  return (structInfo != nullptr) &&
         ((static_cast<int>(structInfo->flag) & static_cast<int>(StructTypeFlag::USER_DEFINED)) != 0);
}

bool TypeDB::isVectorType(type_id_t type_id) const {
  const auto* structInfo = getStructInfo(type_id);
  return (structInfo != nullptr) &&
         ((static_cast<int>(structInfo->flag) & static_cast<int>(StructTypeFlag::LLVM_VECTOR)) != 0);
}

bool TypeDB::isValid(type_id_t type_id) const {
  if (isBuiltinType(type_id)) {
    return true;
  }
  return typeid_to_list_index.find(type_id) != typeid_to_list_index.end();
}

bool TypeDB::isValid(alloc_id_t alloc_id) const {
  return alloc_id.value() > 0 && alloc_id.value() <= static_cast<alloc_id_t::value_type>(allocation_info.size());
}

void TypeDB::registerStruct(const StructTypeInfo& struct_type) {
  if (isValid(struct_type.type_id) || !isStructType(struct_type.type_id)) {
    if (isBuiltinType(struct_type.type_id)) {
      LOG_ERROR("Built-in type ID used for struct " << struct_type.name);
    } else if (isReservedType(struct_type.type_id)) {
      LOG_ERROR("Type ID is reserved for builtin types. Struct: " << struct_type.name);
    } else if (isUnknown(struct_type.type_id)) {
      LOG_ERROR("Type ID is reserved for unknown types. Struct: " << struct_type.name);
    } else {
      LOG_ERROR("Struct type ID already registered for " << struct_type.name << ". Conflicting struct is "
                                                         << getStructInfo(struct_type.type_id)->name);
    }
    return;
  }
  struct_info_vec.push_back(struct_type);
  typeid_to_list_index.insert({struct_type.type_id, struct_info_vec.size() - 1});
}

alloc_id_t TypeDB::getOrCreateAllocationId(type_id_t type_id, std::optional<size_t> count,
                                           std::optional<ptrdiff_t> base_ptr_offset) {
  auto it = std::find_if(allocation_info.begin(), allocation_info.end(), [&](auto& info) {
    return info.type_id == type_id && info.count == count && info.base_ptr_offset == base_ptr_offset;
  });
  if (it != allocation_info.end()) {
    return it->alloc_id;
  }
  AllocationInfo info;
  info.alloc_id        = static_cast<alloc_id_t::value_type>(allocation_info.size() + 1);
  info.type_id         = type_id;
  info.count           = count;
  info.base_ptr_offset = base_ptr_offset;
  allocation_info.push_back(std::move(info));
  return allocation_info.back().alloc_id;
}

void TypeDB::registerAllocations(std::vector<AllocationInfo> allocations) {
  allocation_info = std::move(allocations);
  for (size_t i = 0; i < allocation_info.size(); ++i) {
    assert(allocation_info[i].alloc_id.value() - 1 == static_cast<alloc_id_t::value_type>(i));
  }
}

const std::string& TypeDB::getTypeName(type_id_t type_id) const {
  if (isBuiltinType(type_id)) {
    return BuiltinNames[type_id.value()];
  }
  if (isStructType(type_id)) {
    const auto* structInfo = getStructInfo(type_id);
    if (structInfo != nullptr) {
      return structInfo->name;
    }
  }
  return UnknownStructName;
}

size_t TypeDB::getTypeSize(type_id_t type_id) const {
  if (isReservedType(type_id)) {
    if (isBuiltinType(type_id)) {
      return BuiltinSizes[type_id.value()];
    }
    return 0;
  }

  const auto* structInfo = getStructInfo(type_id);
  if (structInfo != nullptr) {
    return structInfo->extent;
  }
  return 0;
}

const StructTypeInfo* TypeDB::getStructInfo(type_id_t type_id) const {
  const auto index_iter = typeid_to_list_index.find(type_id);
  if (index_iter != typeid_to_list_index.end()) {
    return &struct_info_vec[index_iter->second];
  }
  return nullptr;
}

const AllocationInfo* TypeDB::getAllocationInfo(alloc_id_t alloc_id) const {
  if (alloc_id.value() == 0 || alloc_id.value() > static_cast<alloc_id_t::value_type>(allocation_info.size())) {
    return nullptr;
  }
  return &allocation_info[alloc_id.value() - 1];
}

const std::vector<StructTypeInfo>& TypeDB::getStructList() const {
  return struct_info_vec;
}

const std::vector<AllocationInfo>& TypeDB::getAllocationInfo() const {
  return allocation_info;
}

bool TypeDB::isUnknown(type_id_t type_id) const {
  return type_id == type_id_t::unknown_type;
}

}  // namespace typeart
