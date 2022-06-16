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

#include "meta/Database.hpp"

#include "support/Logger.hpp"

#include <fmt/core.h>
#include <iostream>
#include <set>
#include <stack>
#include <utility>

namespace meta {

Database::Database() {
}

Database::~Database() {
}

[[nodiscard]] meta::Meta* Database::addMeta(std::unique_ptr<meta::Meta> meta) {
  createMappings();
  if (meta.get() == nullptr) {
    LOG_FATAL("Database::addMeta argument must not be nullptr");
    abort();
  }
  if (auto existing_meta = lookupMeta(*meta); existing_meta != nullptr) {
    // TODO make sure that unused Meta instances are discarded.
    replaceRefs(*meta, *existing_meta);
    return existing_meta;
  }
  meta->set_id(reserveMetaId());
  addMappingsFor(*meta);
  return storeMeta(std::move(meta));
}

[[nodiscard]] meta::di::StructureType* Database::lookupStructureType(const std::string& identifier) {
  createMappings();
  if (auto it = structure_store.find(identifier); it != structure_store.end()) {
    return it->second;
  }
  return nullptr;
}

[[nodiscard]] meta::di::Subprogram* Database::lookupSubprogram(const std::string& linkage_name) {
  createMappings();
  if (auto it = subprogram_store.find(linkage_name); it != subprogram_store.end()) {
    return it->second;
  }
  return nullptr;
}

meta::Meta* Database::getMeta(meta_id_t meta_id) {
  if (meta_id == meta_id_t::invalid || meta_id.value() > static_cast<meta_id_t::value_type>(meta_info.size())) {
    return nullptr;
  }
  return meta_info[meta_id.value() - 1].get();
}

const meta::Meta* Database::getMeta(meta_id_t meta_id) const {
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
      if (!info) {
        continue;
      }
      if (meta == *info) {
        return info.get();
      }
    }
  }
  return nullptr;
}

[[nodiscard]] meta::Meta* Database::storeMeta(std::unique_ptr<meta::Meta> meta) {
  const auto id = meta->get_id();
  if (id.value() > static_cast<meta_id_t::value_type>(meta_info.size())) {
    meta_info.resize(id.value());
  }
  auto result               = meta.get();
  meta_info[id.value() - 1] = std::move(meta);
  return result;
}

void Database::replaceRefs(const meta::Meta& original, meta::Meta& replacement) {
  for (auto& info : meta_info) {
    if (!info) {
      continue;
    }
    for (auto& ref : info->get_refs()) {
      if (ref == &original) {
        ref = &replacement;
      }
    }
  }
}

void Database::addMappingsFor(meta::Meta& meta) {
  if (auto meta_string = meta::dyn_cast<meta::String>(&meta)) {
    string_store.try_emplace(meta_string->get_data(), meta_string);
  }
  if (auto structure_type = meta::dyn_cast<meta::di::StructureType>(&meta)) {
    structure_store.try_emplace(structure_type->get_identifier(), structure_type);
  }
  if (auto subprogram = meta::dyn_cast<meta::di::Subprogram>(&meta)) {
    subprogram_store.try_emplace(subprogram->get_linkage_name(), subprogram);
  }
}

void Database::createMappings() {
  // We create mappings lazily as they are only needed when elements are added
  // to the database.
  if (!has_mappings) {
    for (const auto& elem : meta_info) {
      if (!elem) {
        continue;
      }
      addMappingsFor(*elem);
    }
    has_mappings = true;
  }
}

}  // namespace meta
