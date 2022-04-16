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

#include "runtime/TypeResolution.hpp"

#include "runtime/Runtime.h"
#include "support/Logger.hpp"
#include "support/System.hpp"

#include "llvm/ADT/Optional.h"
#include "llvm/Support/raw_ostream.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace typeart::runtime {

// The element type of member_types in typeart_struct_layout must be the same as the value_type of type_id_t
static_assert(std::is_same_v<std::remove_cv_t<std::remove_pointer_t<decltype(typeart_struct_layout::member_types)>>,
                             type_id_t::value_type>);
// The type of type_id in typeart_struct_layout must be the same as the value_type of type_id_t
static_assert(std::is_same_v<decltype(typeart_struct_layout::type_id), type_id_t::value_type>);

template <typename T>
inline const void* addByteOffset(const void* addr, T offset) {
  return static_cast<const void*>(static_cast<const uint8_t*>(addr) + offset);
}

TypeResolution::TypeResolution(const Database& db) : db{db} {
}

const std::string& TypeResolution::getTypeName(type_id_t type_id) const {
  return db.getTypeName(type_id);
}

size_t TypeResolution::getTypeSize(type_id_t type_id) const {
  return db.getTypeSize(type_id);
}

size_t TypeResolution::getMemberIndex(typeart_struct_layout structInfo, size_t offset) const {
  size_t n = structInfo.num_members;
  if (offset > structInfo.offsets[n - 1]) {
    return n - 1;
  }

  size_t i = 0;
  while (i < n - 1 && offset >= structInfo.offsets[i + 1]) {
    ++i;
  }
  return i;
}

TypeResolution::TypeArtStatus TypeResolution::getSubTypeInfo(const void* baseAddr, size_t offset,
                                                             typeart_struct_layout containerInfo,
                                                             type_id_value* subType, const void** subTypeBaseAddr,
                                                             size_t* subTypeOffset, size_t* subTypeCount) const {
  if (offset >= containerInfo.extent) {
    return TYPEART_BAD_OFFSET;
  }

  // Get index of the struct member at the address
  size_t memberIndex = getMemberIndex(containerInfo, offset);

  int memberType = containerInfo.member_types[memberIndex];

  size_t baseOffset = containerInfo.offsets[memberIndex];
  assert(offset >= baseOffset && "Invalid offset values");

  size_t internalOffset   = offset - baseOffset;
  size_t typeSize         = db.getTypeSize(memberType);
  size_t offsetInTypeSize = internalOffset / typeSize;
  size_t newOffset        = internalOffset % typeSize;

  // If newOffset != 0, the subtype cannot be atomic, i.e. must be a struct
  if (newOffset != 0) {
    if (db.isReservedType(memberType)) {
      return TYPEART_BAD_ALIGNMENT;
    }
  }

  // Ensure that the array index is in bounds
  if (offsetInTypeSize >= containerInfo.count[memberIndex]) {
    // Address points to padding
    return TYPEART_BAD_ALIGNMENT;
  }

  *subType         = memberType;
  *subTypeBaseAddr = addByteOffset(baseAddr, baseOffset);
  *subTypeOffset   = newOffset;
  *subTypeCount    = containerInfo.count[memberIndex] - offsetInTypeSize;

  return TYPEART_OK;
}

TypeResolution::TypeArtStatus TypeResolution::getSubTypeInfo(const void* baseAddr, size_t offset,
                                                             const StructType& containerInfo, type_id_value* subType,
                                                             const void** subTypeBaseAddr, size_t* subTypeOffset,
                                                             size_t* subTypeCount) const {
  typeart_struct_layout structLayout;
  structLayout.type_id      = containerInfo.type_id.value();
  structLayout.name         = containerInfo.name.c_str();
  structLayout.num_members  = containerInfo.num_members;
  structLayout.extent       = containerInfo.extent;
  structLayout.offsets      = &containerInfo.offsets[0];
  structLayout.member_types = reinterpret_cast<const type_id_t::value_type*>(&containerInfo.member_types[0]);
  structLayout.count        = &containerInfo.array_sizes[0];
  return getSubTypeInfo(baseAddr, offset, structLayout, subType, subTypeBaseAddr, subTypeOffset, subTypeCount);
}

TypeResolution::TypeArtStatus TypeResolution::getTypeInfoInternal(const void* baseAddr, size_t offset,
                                                                  const StructType& containerInfo, type_id_value* type,
                                                                  size_t* count) const {
  assert(offset < containerInfo.extent && "Something went wrong with the base address computation");

  TypeArtStatus status;
  int subType{-1};
  const void* subTypeBaseAddr;
  size_t subTypeOffset{0};
  size_t subTypeCount{0};
  const StructType* structInfo = &containerInfo;

  bool resolve{true};

  // Resolve type recursively, until the address matches exactly
  while (resolve) {
    status = getSubTypeInfo(baseAddr, offset, *structInfo, &subType, &subTypeBaseAddr, &subTypeOffset, &subTypeCount);

    if (status != TYPEART_OK) {
      return status;
    }

    baseAddr = subTypeBaseAddr;
    offset   = subTypeOffset;

    // Continue as long as there is a byte offset
    resolve = offset != 0;

    // Get layout of the nested struct
    if (resolve) {
      status = getStructType(subType, &structInfo);
      if (status != TYPEART_OK) {
        return status;
      }
    }
  }
  *type  = subType;
  *count = subTypeCount;
  return TYPEART_OK;
}

TypeResolution::TypeArtStatus TypeResolution::getTypeInfo(const void* addr, const PointerInfo& ptrInfo,
                                                          type_id_value* type, size_t* count) const {
  auto containingType = ptrInfo.type_id;
  size_t containingTypeCount{0};
  size_t internalOffset{0};

  // First, retrieve the containing type
  TypeArtStatus status = getContainingTypeInfo(addr, ptrInfo, &containingTypeCount, &internalOffset);
  if (status != TYPEART_OK) {
    return status;
  }

  // Check for exact address match
  if (internalOffset == 0) {
    *type  = containingType.value();
    *count = containingTypeCount;
    return TYPEART_OK;
  }

  if (db.isBuiltinType(containingType)) {
    // Address points to the middle of a builtin type
    return TYPEART_BAD_ALIGNMENT;
  }

  // Resolve struct recursively
  const auto* structInfo = db.getStructType(containingType);
  if (structInfo != nullptr) {
    const void* containingTypeAddr = addByteOffset(addr, -std::ptrdiff_t(internalOffset));
    return getTypeInfoInternal(containingTypeAddr, internalOffset, *structInfo, type, count);
  }
  return TYPEART_INVALID_TYPE_ID;
}

TypeResolution::TypeArtStatus TypeResolution::getContainingTypeInfo(const void* addr, const PointerInfo& ptrInfo,
                                                                    size_t* count, size_t* offset) const {
  const auto& basePtrInfo = ptrInfo;
  auto type_id            = basePtrInfo.type_id;
  size_t typeSize         = db.getTypeSize(type_id);

  // Check for exact match -> no further checks and offsets calculations needed
  if (ptrInfo.base_addr == addr) {
    *count  = ptrInfo.count;
    *offset = 0;
    return TYPEART_OK;
  }

  // The address points inside a known array
  const void* blockEnd = addByteOffset(basePtrInfo.base_addr, basePtrInfo.count * typeSize);

  // Ensure that the given address is in bounds and points to the start of an element
  if (addr >= blockEnd) {
    const std::ptrdiff_t offset2base =
        static_cast<const uint8_t*>(addr) - static_cast<const uint8_t*>(basePtrInfo.base_addr);
    const auto oob_index = (offset2base / typeSize) - basePtrInfo.count + 1;
    LOG_WARNING("Out of bounds for the lookup: (" << toString(addr, ptrInfo) << ") #Elements too far: " << oob_index);
    return TYPEART_UNKNOWN_ADDRESS;
  }

  assert(addr >= basePtrInfo.base_addr && "Error in base address computation");
  size_t addrDif = reinterpret_cast<size_t>(addr) - reinterpret_cast<size_t>(basePtrInfo.base_addr);

  // Offset of the pointer w.r.t. the start of the containing type
  size_t internalOffset = addrDif % typeSize;

  // Array index
  size_t typeOffset = addrDif / typeSize;
  size_t typeCount  = basePtrInfo.count - typeOffset;

  // Retrieve and return type information
  *count  = typeCount;
  *offset = internalOffset;
  return TYPEART_OK;
}

TypeResolution::TypeArtStatus TypeResolution::getBuiltinInfo(const void* addr, const PointerInfo& ptrInfo,
                                                             BuiltinType* type) const {
  if (db.isReservedType(ptrInfo.type_id)) {
    *type = static_cast<BuiltinType>(ptrInfo.type_id.value());
    return TYPEART_OK;
  }
  return TYPEART_WRONG_KIND;
}

TypeResolution::TypeArtStatus TypeResolution::getStructType(type_id_t type_id, const StructType** structInfo) const {
  // Requested ID must correspond to a struct
  if (!db.isStructType(type_id)) {
    return TYPEART_WRONG_KIND;
  }

  const auto* result = db.getStructType(type_id);

  if (result != nullptr) {
    *structInfo = result;
    return TYPEART_OK;
  }
  return TYPEART_INVALID_TYPE_ID;
}

TypeResolution::TypeArtStatus TypeResolution::getAllocationInfo(alloc_id_t alloc_id,
                                                                const AllocationInfo** allocation_info) const {
  auto result = db.getAllocationInfo(alloc_id);
  if (result != nullptr) {
    *allocation_info = result;
    return TYPEART_OK;
  }
  LOG_ERROR("Invalid alloc_id " << alloc_id.value() << "!");
  return TYPEART_INVALID_ALLOC_ID;
}

std::string TypeResolution::toString(const void* addr, type_id_t type_id, size_t count, size_t typeSize,
                                     const void* calledFrom) const {
  std::string buf;
  llvm::raw_string_ostream s(buf);
  const auto name = db.getTypeName(type_id);
  s << addr << " " << type_id.value() << " " << name << " " << typeSize << " " << count << " (" << calledFrom << ")";
  return s.str();
}

std::string TypeResolution::toString(const void* addr, type_id_t type_id, size_t count, const void* calledFrom) const {
  const auto typeSize = db.getTypeSize(type_id);
  return toString(addr, type_id, count, typeSize, calledFrom);
}

std::string TypeResolution::toString(const void* addr, const PointerInfo& pointer_info) const {
  return toString(addr, pointer_info.type_id, pointer_info.count, pointer_info.debug);
}

bool TypeResolution::isUnknown(type_id_t type_id) const {
  return db.isUnknown(type_id);
}

bool TypeResolution::isValid(type_id_t type_id) const {
  return db.isValid(type_id);
}

bool TypeResolution::isReservedType(type_id_t type_id) const {
  return db.isReservedType(type_id);
}

bool TypeResolution::isBuiltinType(type_id_t type_id) const {
  return db.isBuiltinType(type_id);
}

bool TypeResolution::isStructType(type_id_t type_id) const {
  return db.isStructType(type_id);
}

bool TypeResolution::isUserDefinedType(type_id_t type_id) const {
  return db.isUserDefinedType(type_id);
}

bool TypeResolution::isVectorType(type_id_t type_id) const {
  return db.isVectorType(type_id);
}

}  // namespace typeart::runtime
