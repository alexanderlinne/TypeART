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

void Database::addMeta(std::unique_ptr<meta::Meta> meta) {
  if (meta.get() == nullptr) {
    LOG_FATAL("Database::addMeta argument must not be nullptr");
    abort();
  }
  meta->set_id(reserveMetaId());
  storeMeta(std::move(meta));
}

const std::vector<std::unique_ptr<Meta>>& Database::getMeta() const {
  return meta_info;
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

void Database::storeMeta(std::unique_ptr<meta::Meta> meta) {
  const auto id = meta->get_id();
  if (id.value() > static_cast<meta_id_t::value_type>(meta_info.size())) {
    meta_info.resize(id.value());
  }
  meta_info[id.value() - 1] = std::move(meta);
}

}  // namespace meta
