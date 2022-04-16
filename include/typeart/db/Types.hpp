// TypeART library
//
// Copyright (c) 2017-2021 TypeART Authors
// Distributed under the BSD 3-Clause license.
// (See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/BSD-3-Clause)
//
// Project home: https://github.com/tudasc/TypeART
//
// SPDX-License-Identifier: BSD-3-Clause
//

#pragma once

#include "Types.h"

#include <ostream>

namespace typeart {

struct type_id_t {
  using value_type = type_id_value;

  static const type_id_t unknown_type;
  static const type_id_t invalid;

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

}  // namespace typeart

namespace std {

template <>
struct hash<typeart::type_id_t> {
  std::size_t operator()(const typeart::type_id_t& type_id) const {
    return hash<typeart::type_id_t::value_type>{}(type_id.value());
  }
};

}  // namespace std
