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
#include <set>
#include <stack>
#include <utility>

namespace typeart {

Database::Database() {
}

Database::~Database() {
}

alloc_id_t Database::getOrCreateAllocationId(meta_id_t meta_id, std::optional<size_t> count) {
  auto it = std::find_if(allocation_info.begin(), allocation_info.end(),
                         [&](auto& info) { return info.meta_id == meta_id && info.count == count; });
  if (it != allocation_info.end()) {
    return it->alloc_id;
  }
  AllocationInfo info;
  info.alloc_id = static_cast<alloc_id_t::value_type>(allocation_info.size() + 1);
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

const AllocationInfo* Database::getAllocationInfo(alloc_id_t alloc_id) const {
  if (alloc_id.value() == 0 || alloc_id.value() > static_cast<alloc_id_t::value_type>(allocation_info.size())) {
    return nullptr;
  }
  return &allocation_info[alloc_id.value() - 1];
}

[[nodiscard]] meta::Meta* Database::registerMeta(std::unique_ptr<meta::Meta> meta) {
  const auto id = meta->get_id();
  if (id != meta_id_t::invalid && id.value() <= static_cast<meta_id_t::value_type>(meta_info.size()) &&
      meta_info[id.value() - 1]) {
    return nullptr;
  }
  return storeMeta(std::move(meta));
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
      if (ref.get() == nullptr) {
        ref.set(*getMetaInfo(ref.get_id()));
      }
    }
  }
  for (const auto& elem : meta_info) {
    if (auto structure_type = meta::dyn_cast<meta::di::StructureType>(elem.get())) {
      structure_store.try_emplace(structure_type->get_identifier(), structure_type);
    }
    if (auto subprogram = meta::dyn_cast<meta::di::Subprogram>(elem.get())) {
      subprogram_store.try_emplace(subprogram->get_linkage_name(), subprogram);
    }
  }
  return true;
}

[[nodiscard]] meta::Meta* Database::addMeta(std::unique_ptr<meta::Meta> meta) {
  if (meta.get() == nullptr) {
    LOG_FATAL("Database::addMeta argument must not be nullptr");
    abort();
  }
  if (auto existing_meta = lookupMeta(*meta.get()); existing_meta != nullptr) {
    return existing_meta;
  }
  meta->set_id(reserveMetaId());
  if (auto structure_type = meta::dyn_cast<meta::di::StructureType>(meta.get())) {
    structure_store.try_emplace(structure_type->get_identifier(), structure_type);
  }
  if (auto subprogram = meta::dyn_cast<meta::di::Subprogram>(meta.get())) {
    subprogram_store.try_emplace(subprogram->get_linkage_name(), subprogram);
  }
  return storeMeta(std::move(meta));
}

[[nodiscard]] meta::di::StructureType* Database::lookupStructureType(const std::string& identifier) {
  if (auto it = structure_store.find(identifier); it != structure_store.end()) {
    return it->second;
  }
  return nullptr;
}

[[nodiscard]] meta::di::Subprogram* Database::lookupSubprogram(const std::string& linkage_name) {
  if (auto it = subprogram_store.find(linkage_name); it != subprogram_store.end()) {
    return it->second;
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

[[nodiscard]] meta_id_t Database::reserveMetaId() {
  auto result = meta_id_t{static_cast<meta_id_t::value_type>(meta_info.size() + 1)};
  meta_info.resize(result.value());
  return result;
}

[[nodiscard]] meta::Meta* Database::lookupMeta(const meta::Meta& meta) {
  if (auto meta_string = meta::dyn_cast<meta::String>(&meta)) {
    if (auto it = string_store.find(meta_string->get_data()); it != string_store.end()) {
      return it->second;
    }
  } else if (auto structure_type = meta::dyn_cast<meta::di::StructureType>(&meta)) {
    return lookupStructureType(structure_type->get_identifier());
  } else if (auto subprogram = meta::dyn_cast<meta::di::Subprogram>(&meta)) {
    return lookupSubprogram(subprogram->get_linkage_name());
  } else {
    for (const auto& info : meta_info) {
      if (meta == *info) {
        return info.get();
      }
    }
  }
  return nullptr;
}

[[nodiscard]] meta::Meta* Database::storeMeta(std::unique_ptr<meta::Meta> meta) {
  if (auto meta_string = meta::dyn_cast<meta::String>(meta.get())) {
    string_store.try_emplace(meta_string->get_data(), meta_string);
  }
  const auto id = meta->get_id();
  if (id.value() > static_cast<meta_id_t::value_type>(meta_info.size())) {
    meta_info.resize(id.value());
  }
  auto result               = meta.get();
  meta_info[id.value() - 1] = std::move(meta);
  return result;
}

}  // namespace typeart
