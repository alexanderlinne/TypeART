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

#include "db/Database.hpp"

#include "support/Logger.hpp"

#include <fmt/core.h>
#include <iostream>
#include <queue>
#include <set>
#include <utility>

namespace typeart {

bool StructType::isValid() const {
  return type_id != type_id_t::invalid;
}

StructType::operator typeart_struct_layout() const {
  typeart_struct_layout struct_layout;
  struct_layout.type_id      = type_id.value();
  struct_layout.name         = name.c_str();
  struct_layout.num_members  = num_members;
  struct_layout.extent       = extent;
  struct_layout.offsets      = &offsets[0];
  struct_layout.member_types = reinterpret_cast<const type_id_t::value_type*>(&member_types[0]);
  struct_layout.count        = &array_sizes[0];
  return struct_layout;
}

const std::array<std::string, 11> Database::BuiltinNames = {
    "int8", "int16", "int32", "int64", "half", "float", "double", "float128", "x86_float80", "ppc_float128", "pointer"};

// TODO: Builtin ID changes lead tsto wrong type sizes/names
const std::array<size_t, 11> Database::BuiltinSizes = {1,  2,
                                                       4,  8,
                                                       2,  4,
                                                       8,  16,
                                                       16,  // TODO: Always correct?
                                                       16, sizeof(void*)};

const std::string Database::UnknownStructName{"typeart_unknown_struct"};

bool Database::isBuiltinType(type_id_t type_id) const {
  return type_id.value() >= TYPEART_INT8 && type_id.value() < TYPEART_NUM_VALID_IDS;
}

bool Database::isReservedType(type_id_t type_id) const {
  return type_id.value() < TYPEART_NUM_RESERVED_IDS;
}

bool Database::isStructType(type_id_t type_id) const {
  return type_id.value() >= TYPEART_NUM_RESERVED_IDS;
}

bool Database::isUserDefinedType(type_id_t type_id) const {
  const auto* structInfo = getStructType(type_id);
  return (structInfo != nullptr) &&
         ((static_cast<int>(structInfo->flag) & static_cast<int>(StructTypeFlag::USER_DEFINED)) != 0);
}

bool Database::isVectorType(type_id_t type_id) const {
  const auto* structInfo = getStructType(type_id);
  return (structInfo != nullptr) &&
         ((static_cast<int>(structInfo->flag) & static_cast<int>(StructTypeFlag::LLVM_VECTOR)) != 0);
}

bool Database::isValid(type_id_t type_id) const {
  if (isBuiltinType(type_id)) {
    return true;
  }
  return static_cast<type_id_t::value_type>(struct_types.size()) > type_id.value() &&
         struct_types[type_id.value()].type_id != type_id_t::invalid;
}

bool Database::isValid(alloc_id_t alloc_id) const {
  return alloc_id.value() > 0 && alloc_id.value() <= static_cast<alloc_id_t::value_type>(allocation_info.size());
}

void Database::registerStruct(const StructType& struct_type) {
  if (!struct_type.isValid()) {
    LOG_ERROR("Invalid type ID used for struct " << struct_type.name);
    return;
  }
  if (isValid(struct_type.type_id) || !isStructType(struct_type.type_id)) {
    if (isBuiltinType(struct_type.type_id)) {
      LOG_ERROR("Built-in type ID used for struct " << struct_type.name);
    } else if (isReservedType(struct_type.type_id)) {
      LOG_ERROR("Type ID is reserved for builtin types. Struct: " << struct_type.name);
    } else if (isUnknown(struct_type.type_id)) {
      LOG_ERROR("Type ID is reserved for unknown types. Struct: " << struct_type.name);
    } else {
      LOG_ERROR("Struct type ID already registered for " << struct_type.name << ". Conflicting struct is "
                                                         << getStructType(struct_type.type_id)->name);
    }
    return;
  }
  if (static_cast<type_id_t::value_type>(struct_types.size()) <= struct_type.type_id.value()) {
    struct_types.resize(struct_type.type_id.value() + 1);
  }
  struct_types[struct_type.type_id.value()] = struct_type;
}

alloc_id_t Database::getOrCreateAllocationId(type_id_t type_id, meta_id_t meta_id, std::optional<size_t> count) {
  auto it = std::find_if(allocation_info.begin(), allocation_info.end(),
                         [&](auto& info) { return info.type_id == type_id && info.count == count; });
  if (it != allocation_info.end()) {
    return it->alloc_id;
  }
  AllocationInfo info;
  info.alloc_id = static_cast<alloc_id_t::value_type>(allocation_info.size() + 1);
  info.type_id  = type_id;
  info.meta_id  = meta_id;
  info.count    = count;
  allocation_info.push_back(std::move(info));
  return allocation_info.back().alloc_id;
}

void Database::registerAllocations(std::vector<AllocationInfo> allocations) {
  allocation_info = std::move(allocations);
  for (size_t i = 0; i < allocation_info.size(); ++i) {
    assert(allocation_info[i].alloc_id.value() - 1 == static_cast<alloc_id_t::value_type>(i));
  }
}

const std::string& Database::getTypeName(type_id_t type_id) const {
  if (isBuiltinType(type_id)) {
    return BuiltinNames[type_id.value()];
  }
  if (isStructType(type_id)) {
    const auto* structInfo = getStructType(type_id);
    if (structInfo != nullptr) {
      return structInfo->name;
    }
  }
  return UnknownStructName;
}

size_t Database::getTypeSize(type_id_t type_id) const {
  if (isReservedType(type_id)) {
    if (isBuiltinType(type_id)) {
      return BuiltinSizes[type_id.value()];
    }
    return 0;
  }

  const auto* structInfo = getStructType(type_id);
  if (structInfo != nullptr) {
    return structInfo->extent;
  }
  return 0;
}

const StructType* Database::getStructType(type_id_t type_id) const {
  if (isValid(type_id) && struct_types[type_id.value()].isValid()) {
    return &struct_types[type_id.value()];
  }
  return nullptr;
}

const std::vector<StructType>& Database::getStructTypes() const {
  return struct_types;
}

const AllocationInfo* Database::getAllocationInfo(alloc_id_t alloc_id) const {
  if (alloc_id.value() == 0 || alloc_id.value() > static_cast<alloc_id_t::value_type>(allocation_info.size())) {
    return nullptr;
  }
  return &allocation_info[alloc_id.value() - 1];
}

[[nodiscard]] meta_id_t Database::reserveMetaId() {
  auto result = meta_id_t{static_cast<meta_id_t::value_type>(meta_info.size() + 1)};
  meta_info.resize(result.value());
  return result;
}

[[nodiscard]] std::optional<meta::Ref<meta::Meta>> Database::registerMeta(std::unique_ptr<meta::Meta> meta) {
  if (!meta) {
    return {meta::Ref<meta::Meta>{}};
  }
  const auto id = meta->get_id();
  if (id != meta_id_t::invalid && id.value() <= static_cast<meta_id_t::value_type>(meta_info.size()) &&
      meta_info[id.value() - 1]) {
    return {};
  }
  if (auto meta_string = meta::dyn_cast<meta::String>(meta.get())) {
    string_store.try_emplace(meta_string->get_data(), meta_string);
  }
  return storeMeta(std::move(meta));
}

[[nodiscard]] meta::Ref<meta::Meta> Database::addMeta(std::unique_ptr<meta::Meta> meta) {
  meta->set_id(reserveMetaId());
  if (auto meta_string = meta::dyn_cast<meta::String>(meta.get())) {
    string_store.try_emplace(meta_string->get_data(), meta_string);
  }
  return storeMeta(std::move(meta));
}

[[nodiscard]] meta::Ref<meta::Meta> Database::storeMeta(std::unique_ptr<meta::Meta> meta) {
  const auto id = meta->get_id();
  if (id.value() > static_cast<meta_id_t::value_type>(meta_info.size())) {
    meta_info.resize(id.value());
  }
  auto result               = meta.get();
  meta_info[id.value() - 1] = std::move(meta);
  return {result, result->get_kind()};
}

[[nodiscard]] bool Database::registerMeta(std::vector<std::unique_ptr<meta::Meta>> meta) {
  for (auto&& elem : meta) {
    if (!elem) {
      continue;
    }
    auto id = elem->get_id();
    if (!registerMeta(std::move(elem))) {
      fmt::print(stderr, "Duplicate meta id {} found!\n", id.value());
      abort();
    }
  }
  for (const auto& elem : meta_info) {
    if (!elem) {
      continue;
    }
    for (auto& ref : elem->get_refs()) {
      if (ref.get_meta() == nullptr) {
        ref.set_meta(getMetaInfo(ref.get_id()));
      }
    }
  }
  return true;
}

[[nodiscard]] meta::Meta* Database::lookupMeta(meta::Meta* meta) {
  if (auto meta_string = meta::dyn_cast<meta::String>(meta)) {
    if (auto it = string_store.find(meta_string->get_data()); it != string_store.end()) {
      return it->second;
    }
  } else {
    for (const auto& info : meta_info) {
      if (*info == *meta) {
        return info.get();
      }
    }
  }
  return nullptr;
}

meta::Meta* Database::getMetaInfo(meta_id_t meta_id) {
  if (meta_id == meta_id_t::invalid || meta_id.value() > static_cast<meta_id_t::value_type>(meta_info.size())) {
    return nullptr;
  }
  return meta_info[meta_id.value() - 1].get();
}

const meta::Meta* Database::getMetaInfo(meta_id_t meta_id) const {
  if (meta_id == meta_id_t::invalid || meta_id.value() > static_cast<meta_id_t::value_type>(meta_info.size())) {
    return nullptr;
  }
  return meta_info[meta_id.value() - 1].get();
}

bool Database::isUnknown(type_id_t type_id) const {
  return type_id == type_id_t::unknown_type;
}

}  // namespace typeart
