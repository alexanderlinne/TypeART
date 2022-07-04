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

  Database(const Database&)            = delete;
  Database& operator=(const Database&) = delete;

  Database(Database&&)            = default;
  Database& operator=(Database&&) = default;

  void addMeta(std::unique_ptr<meta::Meta> meta);

  [[nodiscard]] const std::vector<std::unique_ptr<Meta>>& getMeta() const;
  [[nodiscard]] meta::Meta* getMeta(meta_id_t meta_id);
  [[nodiscard]] const meta::Meta* getMeta(meta_id_t meta_id) const;

 private:
  [[nodiscard]] meta_id_t reserveMetaId();
  void storeMeta(std::unique_ptr<meta::Meta> meta);

  std::vector<std::unique_ptr<meta::Meta>> meta_info;
};

}  // namespace meta
