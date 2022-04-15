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

#ifndef TYPEART_VECTORTYPEHANDLER_H
#define TYPEART_VECTORTYPEHANDLER_H

#include "typelib/TypeDatabase.hpp"

#include "llvm/ADT/None.h"
#include "llvm/ADT/StringMap.h"

#include <string>

namespace llvm {
class Type;
class VectorType;
class DataLayout;
}  // namespace llvm

namespace typeart {

class TypeDatabase;
class TypeGenerator;

struct VectorTypeHandler {
  // To avoid problems with padding bytes due to alignment, vector types are represented as structs rather than static
  // arrays. They are given special names and are marked with a TA_VEC flag to avoid confusion.

  const llvm::StringMap<type_id_t>* m_struct_map;
  const TypeDatabase* m_type_db;

  llvm::VectorType* type;
  const llvm::DataLayout& dl;
  const TypeGenerator& m;

  struct VectorData {
    std::string vec_name;
    size_t vector_bytes{0};
    size_t vector_size{0};
  };

  struct ElementData {
    type_id_t element_id     = type_id_t::unknown_type;
    llvm::Type* element_type = nullptr;
    std::string element_name;
  };

  static llvm::Type* getElementType(llvm::VectorType* type);

  [[nodiscard]] llvm::Optional<VectorData> getVectorData() const;

  [[nodiscard]] llvm::Optional<ElementData> getElementData() const;

  [[nodiscard]] llvm::Optional<type_id_t> getElementID() const;

  [[nodiscard]] std::string getName(const ElementData& data) const;

  [[nodiscard]] llvm::Optional<type_id_t> getID() const;
};

}  // namespace typeart

#endif  // TYPEART_VECTORTYPEHANDLER_H
