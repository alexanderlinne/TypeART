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

#include "Meta.hpp"

#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

namespace meta {

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

  [[nodiscard]] meta::Meta* registerMeta(std::unique_ptr<meta::Meta> meta);
  [[nodiscard]] bool registerMeta(std::vector<std::unique_ptr<meta::Meta>> meta);
  [[nodiscard]] meta::Meta* addMeta(std::unique_ptr<meta::Meta> meta);
  [[nodiscard]] meta::di::StructureType* lookupStructureType(const std::string& identifier);
  [[nodiscard]] meta::di::Subprogram* lookupSubprogram(const std::string& linkage_name);

  [[nodiscard]] meta::Meta* getMeta(meta_id_t meta_id);
  [[nodiscard]] const meta::Meta* getMeta(meta_id_t meta_id) const;

 private:
  [[nodiscard]] meta_id_t reserveMetaId();
  [[nodiscard]] meta::Meta* lookupMeta(const meta::Meta& meta);
  [[nodiscard]] meta::Meta* storeMeta(std::unique_ptr<meta::Meta> meta);
  void replaceRefs(const meta::Meta& original, meta::Meta& replacement);

 private:
  std::unordered_map<std::string, meta::String*> string_store;
  std::unordered_map<std::string, meta::di::StructureType*> structure_store;
  std::unordered_map<std::string, meta::di::Subprogram*> subprogram_store;
  std::vector<std::unique_ptr<meta::Meta>> meta_info;
};

}  // namespace meta
