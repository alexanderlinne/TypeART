
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

#include "runtime/Runtime.hpp"

#ifdef TYPEART_BTREE
#ifdef TYPEART_ABSEIL
#error TypeART-RT: Set BTREE and ABSL, mutually exclusive.
#endif
#include "btree_map.h"
#endif

#ifdef TYPEART_ABSEIL
#ifdef TYPEART_BTREE
#error TypeART-RT: Set ABSL and BTREE, mutually exclusive.
#endif
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include "absl/container/btree_map.h"
#pragma GCC diagnostic pop
#endif

#if !defined(TYPEART_BTREE) && !defined(TYPEART_ABSEIL)
#include <map>
#endif

#ifdef USE_SAFEPTR
#ifdef TYPEART_DISABLE_THREAD_SAFETY
#error TypeART-RT: Safe_ptr and disabled thread safety illegal
#endif
#include "safe_ptr.h"
#endif

#include <cstddef>  // size_t
#include <vector>

namespace typeart::runtime::tracker {

struct RuntimeT {
  using Stack = std::vector<const void*>;
  static constexpr auto StackReserve{512U};
  static constexpr char StackName[] = "std::vector";
#ifdef TYPEART_BTREE
  using PointerMapBaseT           = btree::btree_map<const void*, PointerInfo>;
  static constexpr char MapName[] = "btree::btree_map";
#endif
#ifdef TYPEART_ABSEIL
  using PointerMapBaseT           = absl::btree_map<const void*, PointerInfo>;
  static constexpr char MapName[] = "absl::btree_map";
#endif
#if !defined(TYPEART_BTREE) && !defined(TYPEART_ABSEIL)
  using PointerMapBaseT           = std::map<const void*, PointerInfo>;
  static constexpr char MapName[] = "std::map";
#endif
#ifdef USE_SAFEPTR
  using PointerMap = sf::contfree_safe_ptr<PointerMapBaseT>;
  static constexpr bool has_safe_map{true};
#else
  using PointerMap = PointerMapBaseT;
  static constexpr bool has_safe_map{false};
#endif
  using MapEntry   = PointerMapBaseT::value_type;
  using MappedType = PointerMapBaseT::mapped_type;
  using MapKey     = PointerMapBaseT::key_type;
  using StackEntry = Stack::value_type;
};

}  // namespace typeart::runtime::tracker
