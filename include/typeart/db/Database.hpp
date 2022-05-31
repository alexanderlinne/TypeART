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

#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

namespace typeart {

struct alloc_id_t {
  using value_type = alloc_id_value;

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

struct AllocationInfo {
  alloc_id_t alloc_id = alloc_id_t::invalid;
  meta_id_t meta_id   = meta_id_t::invalid;

  // This may be used by allocations with a fixed number of elements.
  std::optional<size_t> count;
};

class Database {
 public:
  [[nodiscard]] static std::optional<Database> load(const std::string& file);
  [[nodiscard]] bool store(const std::string& file);

 public:
  Database();
  ~Database();

  Database(const Database&) = delete;
  Database& operator=(const Database&) = delete;

  Database(Database&&) = default;
  Database& operator=(Database&&) = default;

  [[nodiscard]] alloc_id_t getOrCreateAllocationId(meta_id_t meta_id, std::optional<size_t> count);

  void registerAllocations(std::vector<AllocationInfo> allocation_info);
  [[nodiscard]] const AllocationInfo* getAllocationInfo(alloc_id_t alloc_id) const;

  [[nodiscard]] meta::Meta* registerMeta(std::unique_ptr<meta::Meta> meta);
  [[nodiscard]] bool registerMeta(std::vector<std::unique_ptr<meta::Meta>> meta);
  [[nodiscard]] meta::Meta* addMeta(std::unique_ptr<meta::Meta> meta);
  [[nodiscard]] meta::di::StructureType* lookupStructureType(const std::string& identifier);
  [[nodiscard]] meta::di::Subprogram* lookupSubprogram(const std::string& linkage_name);

  [[nodiscard]] meta::Meta* getMetaInfo(meta_id_t meta_id);
  [[nodiscard]] const meta::Meta* getMetaInfo(meta_id_t meta_id) const;

 private:
  [[nodiscard]] meta_id_t reserveMetaId();
  [[nodiscard]] meta::Meta* lookupMeta(const meta::Meta& meta);
  [[nodiscard]] meta::Meta* storeMeta(std::unique_ptr<meta::Meta> meta);

 private:
  std::vector<AllocationInfo> allocation_info;
  std::unordered_map<std::string, meta::String*> string_store;
  std::unordered_map<std::string, meta::di::StructureType*> structure_store;
  std::unordered_map<std::string, meta::di::Subprogram*> subprogram_store;
  std::vector<std::unique_ptr<meta::Meta>> meta_info;
};

}  // namespace typeart
