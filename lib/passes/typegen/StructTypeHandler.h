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

#ifndef TYPEART_STRUCTTYPEHANDLER_H
#define TYPEART_STRUCTTYPEHANDLER_H

#include "db/Database.hpp"

#include "llvm/ADT/None.h"
#include "llvm/ADT/StringMap.h"

#include <string>

namespace llvm {
class StructType;
}  // namespace llvm

namespace typeart {

struct StructTypeHandler {
  const llvm::StringMap<type_id_t>* m_struct_map;
  const Database* m_type_db;
  llvm::StructType* type;

  [[nodiscard]] static std::string getName(llvm::StructType* type);

  [[nodiscard]] std::string getName() const;

  [[nodiscard]] llvm::Optional<type_id_t> getID() const;
};

}  // namespace typeart

#endif  // TYPEART_STRUCTTYPEHANDLER_H
