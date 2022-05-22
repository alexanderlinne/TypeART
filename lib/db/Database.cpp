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

thread_local Database::IsomorphismCheckBuffers Database::buffers = {};

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
  if (auto meta_string = meta::dyn_cast<meta::String>(meta.get())) {
    string_store.try_emplace(meta_string->get_data(), meta_string);
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
  return true;
}

[[nodiscard]] meta::Meta* Database::addMeta(std::unique_ptr<meta::Meta> meta) {
  if (meta.get() == nullptr) {
    LOG_FATAL("Database::addMeta argument must not be nullptr");
    abort();
  }
  for (const auto& ref : meta->get_refs()) {
    if (!ref.is_valid() && !ref.is_weak()) {
      LOG_FATAL("Database::addMeta cannot add meta with invalid strong refs");
      abort();
    }
    if (ref.get() == nullptr) {
      LOG_FATAL("Database::addMeta cannot add meta with nullptr refs");
      abort();
    }
    if (getMetaInfo(ref.get_id()) != ref.get() && !ref.is_weak()) {
      LOG_FATAL("Database::addMeta cannot add meta with strong refs not known to the database");
      abort();
    }
  }
  if (auto existing_meta = lookupMeta(*meta.get()); existing_meta != nullptr) {
    return existing_meta;
  }
  meta->set_id(reserveMetaId());
  if (auto meta_string = meta::dyn_cast<meta::String>(meta.get())) {
    string_store.try_emplace(meta_string->get_data(), meta_string);
  }
  return storeMeta(std::move(meta));
}

void Database::removeOrphanedMeta() {
  bool deleted_meta   = true;
  auto referenced_ids = std::set<meta_id_value>{};
  while (deleted_meta) {
    deleted_meta = false;
    referenced_ids.clear();
    for (const auto& info : meta_info) {
      if (!info) {
        continue;
      }
      for (const auto& ref : info->get_refs()) {
        if (!ref.is_weak()) {
          referenced_ids.insert(ref.get_id().value());
        }
      }
    }
    for (auto& info : meta_info) {
      if (!info || info->is_retained()) {
        continue;
      }
      if (referenced_ids.find(info->get_id().value()) == referenced_ids.end()) {
        info.reset();
        deleted_meta = true;
      }
    }
  }
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
  } else {
    return findIsomorphicMeta(meta);
  }
  return nullptr;
}

[[nodiscard]] bool checkIsIsomorphic(meta::Ref<const meta::Meta> lhs, meta::Ref<const meta::Meta> rhs,
                                        bool& contains_weak_refs,
                                        std::vector<std::pair<const meta::Meta*, const meta::Meta*>>& parents,
                                        std::unordered_map<const meta::Meta*, const meta::Meta*>& weak_mappings) {
  if (lhs.is_weak() != rhs.is_weak() || lhs->get_kind() != rhs->get_kind() ||
      lhs->get_refs().size() != rhs->get_refs().size()) {
    return false;
  }
  if (lhs.get_kind() == meta::Kind::Integer || lhs.get_kind() == meta::Kind::String) {
    return *lhs.get() == *rhs.get();
  }
  contains_weak_refs |= lhs.is_weak() || rhs.is_weak();
  if (lhs.get() == rhs.get()) {
    return true;
  }
  if (lhs.is_weak()) {
    if (auto it = std::find(parents.begin(), parents.end(), std::pair{lhs.get(), rhs.get()}); it == parents.end()) {
      return false;
    }
    if (auto it = weak_mappings.find(lhs.get()); it != weak_mappings.end()) {
      return rhs.get() == it->second;
    } else {
      weak_mappings.emplace(lhs.get(), rhs.get());
      return true;
    }
  }
  parents.emplace_back(lhs.get(), rhs.get());
  for (size_t i = 0; i < lhs->get_refs().size(); ++i) {
    bool subtree_contains_weak_refs = false;
    if (!checkIsIsomorphic(lhs->get_refs()[i], rhs->get_refs()[i], subtree_contains_weak_refs, parents,
                              weak_mappings)) {
      return false;
    }
    contains_weak_refs |= subtree_contains_weak_refs;
  }
  parents.pop_back();
  if (!contains_weak_refs) {
    return true;
  }
  if (auto it = weak_mappings.find(lhs.get()); it != weak_mappings.end()) {
    return rhs.get() == it->second;
  } else {
    return contains_weak_refs;
  }
}

[[nodiscard]] meta::Meta* Database::findIsomorphicMeta(const meta::Meta& meta) {
  for (const auto& info : meta_info) {
    if (!info) {
      continue;
    }
    bool contains_weak_refs = false;
    buffers.parents.clear();
    buffers.weak_mappings.clear();
    if (checkIsIsomorphic(*info, meta, contains_weak_refs, buffers.parents, buffers.weak_mappings)) {
      replaceWeakRefs(&meta, info.get());
      return info.get();
    }
  }
  return nullptr;
}

void Database::replaceWeakRefs(const meta::Meta* current, meta::Meta* value) {
  for (const auto& info : meta_info) {
    if (!info) {
      continue;
    }
    for (auto& ref : info->get_refs()) {
      if (ref.get() == current) {
        if (!ref.is_weak()) {
          LOG_FATAL("non-weak ref created before registration");
          abort();
        }
        ref.set(*value);
      }
    }
  }
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

}  // namespace typeart
