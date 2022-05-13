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

#ifndef TYPEART_TYPEGENERATOR_H
#define TYPEART_TYPEGENERATOR_H

#include "db/Database.hpp"
#include "db/LLVMMetadataConverter.hpp"

#include <memory>
#include <string>
#include <system_error>
#include <utility>

namespace llvm {
class Type;
class DataLayout;
}  // namespace llvm

namespace typeart {

class TypeGenerator {
 public:
  [[nodiscard]] virtual type_id_t getOrRegisterType(llvm::Type* type, const llvm::DataLayout& layout) = 0;

  [[nodiscard]] virtual type_id_t getTypeID(llvm::Type* type, const llvm::DataLayout& layout) const = 0;

  [[nodiscard]] virtual Database& getDatabase()                     = 0;
  [[nodiscard]] virtual const Database& getDatabase() const         = 0;
  [[nodiscard]] virtual meta::LLVMMetadataConverter& getConverter() = 0;

  [[nodiscard]] virtual bool load() = 0;

  [[nodiscard]] virtual bool store() const = 0;

  virtual ~TypeGenerator() = default;
};

// This doesn't immediately load the file, call load/store after
std::unique_ptr<TypeGenerator> make_typegen(const std::string& file);

}  // namespace typeart

#endif  // TYPEART_TYPEGENERATOR_H
