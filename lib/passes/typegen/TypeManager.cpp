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

#include "TypeManager.h"

#include "StructTypeHandler.h"
#include "VectorTypeHandler.h"
#include "db/Database.hpp"
#include "support/Logger.hpp"
#include "support/TypeUtil.h"
#include "support/Util.h"

#include "llvm/ADT/None.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/TypeSize.h"
#include "llvm/Support/raw_ostream.h"

#include <cassert>
#include <utility>
#include <vector>

namespace typeart {

std::unique_ptr<TypeGenerator> make_typegen(const std::string& file) {
  return std::make_unique<TypeManager>(file);
}

using namespace llvm;

llvm::Optional<type_id_t> get_builtin_typeid(llvm::Type* type) {
  auto& c = type->getContext();

  auto result = typeart_builtin_type::TYPEART_UNKNOWN_TYPE;
  switch (type->getTypeID()) {
    case llvm::Type::IntegerTyID: {
      if (type == Type::getInt8Ty(c)) {
        result = TYPEART_INT8;
      } else if (type == Type::getInt16Ty(c)) {
        result = TYPEART_INT16;
      } else if (type == Type::getInt32Ty(c)) {
        result = TYPEART_INT32;
      } else if (type == Type::getInt64Ty(c)) {
        result = TYPEART_INT64;
      } else {
        result = TYPEART_UNKNOWN_TYPE;
      }
      break;
    }
    case llvm::Type::HalfTyID:
      result = TYPEART_HALF;
      break;
    case llvm::Type::FloatTyID:
      result = TYPEART_FLOAT;
      break;
    case llvm::Type::DoubleTyID:
      result = TYPEART_DOUBLE;
      break;
    case llvm::Type::FP128TyID:
      result = TYPEART_FP128;
      break;
    case llvm::Type::X86_FP80TyID:
      result = TYPEART_X86_FP80;
      break;
    case llvm::Type::PPC_FP128TyID:
      result = TYPEART_PPC_FP128;
      break;
    case llvm::Type::PointerTyID:
      result = TYPEART_POINTER;
      break;
    default:
      return None;
  }
  return {static_cast<type_id_t::value_type>(result)};
}

type_id_t TypeManager::getOrRegisterVector(llvm::VectorType* type, const llvm::DataLayout& dl) {
  namespace tu = typeart::util;

  VectorTypeHandler handler{&structMap, &db, type, dl, *this};
  const auto type_id = handler.getID();
  if (type_id) {
    return type_id.getValue();
  }

  // Type is not registered - reserve new ID and create struct info object:

  auto element_data = handler.getElementData();
  if (!element_data) {
    return type_id_t::unknown_type;
  }

  auto vector_data = handler.getVectorData();
  if (!vector_data) {
    return type_id_t::unknown_type;
  }

  const auto id = reserveNextId();

  const auto [element_id, element_type, element_name] = element_data.getValue();
  const auto [vector_name, vector_bytes, vector_size] = vector_data.getValue();

  std::vector<type_id_t> memberTypeIDs{element_id};
  std::vector<size_t> arraySizes{vector_size};
  std::vector<size_t> offsets{0};

  size_t elementSize = tu::type::getTypeSizeInBytes(element_type, dl);
  size_t usableBytes = vector_size * elementSize;

  // Add padding bytes explicitly
  if (vector_bytes > usableBytes) {
    size_t padding = vector_bytes - usableBytes;
    memberTypeIDs.push_back(static_cast<type_id_t::value_type>(TYPEART_INT8));
    arraySizes.push_back(padding);
    offsets.push_back(usableBytes);
  }

  StructType vecTypeInfo{id,      vector_name,   vector_bytes, memberTypeIDs.size(),
                         offsets, memberTypeIDs, arraySizes,   StructTypeFlag::LLVM_VECTOR};
  db.registerStruct(vecTypeInfo);
  structMap.insert({vector_name, id});
  return id;
}

TypeManager::TypeManager(std::string file)
    : file(std::move(file)), converter(std::make_unique<meta::LLVMMetadataConverter>(db)), structCount(0) {
}

Database& TypeManager::getDatabase() {
  return db;
}

[[nodiscard]] meta::LLVMMetadataConverter& TypeManager::getConverter() {
  return *converter.get();
}

const Database& TypeManager::getDatabase() const {
  return db;
}

bool TypeManager::load() {
  auto database = Database::load(file);
  if (!database.has_value()) {
    return false;
  }
  db        = std::move(database).value();
  converter = std::make_unique<meta::LLVMMetadataConverter>(db);
  structMap.clear();
  for (const auto& structInfo : db.getStructTypes()) {
    if (structInfo.isValid()) {
      structMap.insert({structInfo.name, structInfo.type_id});
    }
  }
  structCount = structMap.size();
  return true;
}

bool TypeManager::store() const {
  return db.store(file);
}

type_id_t TypeManager::getTypeID(llvm::Type* type, const DataLayout& dl) const {
  auto builtin_id = get_builtin_typeid(type);
  if (builtin_id) {
    return builtin_id.getValue();
  }

  switch (type->getTypeID()) {
#if LLVM_VERSION_MAJOR < 11
    case llvm::Type::VectorTyID:
#else
    case llvm::Type::FixedVectorTyID:
#endif
    {
      VectorTypeHandler handle{&structMap, &db, dyn_cast<VectorType>(type), dl, *this};
      const auto type_id = handle.getID();
      if (type_id) {
        return type_id.getValue();
      }
      break;
    }
    case llvm::Type::StructTyID: {
      StructTypeHandler handle{&structMap, &db, dyn_cast<llvm::StructType>(type)};
      const auto type_id = handle.getID();
      if (type_id) {
        return type_id.getValue();
      }
      break;
    }
#if LLVM_VERSION_MAJOR > 10
    case llvm::Type::ScalableVectorTyID: {
      LOG_ERROR("Scalable SIMD vectors are unsupported.")
      break;
    }
#endif
    default:
      break;
  }

  return type_id_t::unknown_type;
}

type_id_t TypeManager::getOrRegisterType(llvm::Type* type, const llvm::DataLayout& dl) {
  auto builtin_id = get_builtin_typeid(type);
  if (builtin_id) {
    return builtin_id.getValue();
  }

  switch (type->getTypeID()) {
#if LLVM_VERSION_MAJOR < 11
    case llvm::Type::VectorTyID:
#else
    case llvm::Type::FixedVectorTyID:
#endif
    {
      return getOrRegisterVector(dyn_cast<VectorType>(type), dl);
    }
    case llvm::Type::StructTyID:
      return getOrRegisterStruct(dyn_cast<llvm::StructType>(type), dl);
    default:
      break;
  }
  return type_id_t::unknown_type;
}

type_id_t TypeManager::getOrRegisterStruct(llvm::StructType* type, const llvm::DataLayout& dl) {
  namespace tu = typeart::util;

  StructTypeHandler handle{&structMap, &db, type};
  const auto type_id = handle.getID();
  if (type_id) {
    return type_id.getValue();
  }

  const auto name = handle.getName();

  // Get next ID and register struct:
  const auto id = reserveNextId();

  size_t n = type->getStructNumElements();

  std::vector<size_t> offsets;
  std::vector<size_t> arraySizes;
  std::vector<type_id_t> memberTypeIDs;

  const StructLayout* layout = dl.getStructLayout(type);

  for (unsigned i = 0; i < n; ++i) {
    llvm::Type* memberType = type->getStructElementType(i);
    auto memberID          = type_id_t::unknown_type;
    size_t arraySize       = 1;

    if (memberType->isArrayTy()) {
      // Note that clang does not allow VLAs inside of structs (GCC does)
      arraySize  = tu::type::getArrayLengthFlattened(memberType);
      memberType = tu::type::getArrayElementType(memberType);
    }

    if (memberType->isStructTy()) {
      if (StructTypeHandler::getName(llvm::dyn_cast<llvm::StructType>(memberType)) == name) {
        memberID = id;
      } else {
        memberID = getOrRegisterType(memberType, dl);
      }
    } else if (memberType->isSingleValueType() || memberType->isPointerTy()) {
      memberID = getOrRegisterType(memberType, dl);
    } else {
      // clang-format off
      LOG_ERROR("In struct: " << tu::dump(*type)
                  << ": Encountered unhandled type: " << tu::dump(*memberType)
                  << " with type id: " << memberType->getTypeID());
      // clang-format on
      assert(false && "Encountered unhandled type");
    }

    size_t offset = layout->getElementOffset(i);
    offsets.push_back(offset);
    arraySizes.push_back(arraySize);
    memberTypeIDs.push_back(memberID);
  }

  size_t numBytes = layout->getSizeInBytes();

  StructType structInfo{id, name, numBytes, n, offsets, memberTypeIDs, arraySizes, StructTypeFlag::USER_DEFINED};
  db.registerStruct(structInfo);

  structMap.insert({name, id});
  return id;
}

type_id_t TypeManager::reserveNextId() {
  auto id = static_cast<size_t>(TYPEART_NUM_RESERVED_IDS) + structCount;
  structCount++;
  return static_cast<type_id_t::value_type>(id);
}

}  // namespace typeart
