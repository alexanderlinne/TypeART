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

#ifndef LLVM_MUST_SUPPORT_TYPEMANAGER_H
#define LLVM_MUST_SUPPORT_TYPEMANAGER_H

#include "TypeGenerator.h"
#include "typelib/TypeDB.hpp"

#include "llvm/ADT/StringMap.h"

#include <cstddef>
#include <string>

namespace llvm {
class DataLayout;
class StructType;
class Type;
class VectorType;
}  // namespace llvm

namespace typeart {

class TypeManager final : public TypeGenerator {
  std::string file;
  TypeDB typeDB;
  llvm::StringMap<type_id_t> structMap;
  size_t structCount;

 public:
  explicit TypeManager(std::string file);
  [[nodiscard]] std::pair<bool, std::error_code> load() override;
  [[nodiscard]] std::pair<bool, std::error_code> store() const override;
  [[nodiscard]] type_id_t getOrRegisterType(llvm::Type* type, const llvm::DataLayout& dl) override;
  [[nodiscard]] type_id_t getTypeID(llvm::Type* type, const llvm::DataLayout& dl) const override;
  [[nodiscard]] const TypeDatabase& getTypeDatabase() const override;
  [[nodiscard]] alloc_id_t getOrRegisterAllocation(type_id_t type_id, std::optional<size_t> count,
                                                   std::optional<ptrdiff_t> base_ptr_offset) override;

 private:
  [[nodiscard]] type_id_t getOrRegisterStruct(llvm::StructType* type, const llvm::DataLayout& dl);
  [[nodiscard]] type_id_t getOrRegisterVector(llvm::VectorType* type, const llvm::DataLayout& dl);
  [[nodiscard]] type_id_t reserveNextId();
};

}  // namespace typeart

#endif  // LLVM_MUST_SUPPORT_TYPEMANAGER_H
