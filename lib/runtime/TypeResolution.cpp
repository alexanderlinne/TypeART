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

#include "TypeResolution.h"

#include "Runtime.h"
#include "RuntimeData.h"
#include "support/Logger.h"
#include "support/System.h"

#include "llvm/ADT/Optional.h"
#include "llvm/Support/raw_ostream.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace typeart {

template <typename T>
inline const void* addByteOffset(const void* addr, T offset) {
  return static_cast<const void*>(static_cast<const uint8_t*>(addr) + offset);
}

TypeResolution::TypeResolution(const TypeDB& db) : typeDB{db} {
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
                                                             typeart_struct_layout containerInfo, int* subType,
                                                             const void** subTypeBaseAddr, size_t* subTypeOffset,
                                                             size_t* subTypeCount) const {
  if (offset >= containerInfo.extent) {
    return TYPEART_BAD_OFFSET;
  }

  // Get index of the struct member at the address
  size_t memberIndex = getMemberIndex(containerInfo, offset);

  int memberType = containerInfo.member_types[memberIndex];

  size_t baseOffset = containerInfo.offsets[memberIndex];
  assert(offset >= baseOffset && "Invalid offset values");

  size_t internalOffset   = offset - baseOffset;
  size_t typeSize         = typeDB.getTypeSize(memberType);
  size_t offsetInTypeSize = internalOffset / typeSize;
  size_t newOffset        = internalOffset % typeSize;

  // If newOffset != 0, the subtype cannot be atomic, i.e. must be a struct
  if (newOffset != 0) {
    if (typeDB.isReservedType(memberType)) {
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
                                                             const StructTypeInfo& containerInfo, int* subType,
                                                             const void** subTypeBaseAddr, size_t* subTypeOffset,
                                                             size_t* subTypeCount) const {
  typeart_struct_layout structLayout;
  structLayout.type_id      = containerInfo.type_id;
  structLayout.name         = containerInfo.name.c_str();
  structLayout.num_members  = containerInfo.num_members;
  structLayout.extent       = containerInfo.extent;
  structLayout.offsets      = &containerInfo.offsets[0];
  structLayout.member_types = &containerInfo.member_types[0];
  structLayout.count        = &containerInfo.array_sizes[0];
  return getSubTypeInfo(baseAddr, offset, structLayout, subType, subTypeBaseAddr, subTypeOffset, subTypeCount);
}

TypeResolution::TypeArtStatus TypeResolution::getTypeInfoInternal(const void* baseAddr, size_t offset,
                                                                  const StructTypeInfo& containerInfo, int* type,
                                                                  size_t* count) const {
  assert(offset < containerInfo.extent && "Something went wrong with the base address computation");

  TypeArtStatus status;
  int subType{-1};
  const void* subTypeBaseAddr;
  size_t subTypeOffset{0};
  size_t subTypeCount{0};
  const StructTypeInfo* structInfo = &containerInfo;

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
      status = getStructInfo(subType, &structInfo);
      if (status != TYPEART_OK) {
        return status;
      }
    }
  }
  *type  = subType;
  *count = subTypeCount;
  return TYPEART_OK;
}

TypeResolution::TypeArtStatus TypeResolution::getTypeInfo(const void* addr, const void* basePtr,
                                                          const PointerInfo& ptrInfo, int* type, size_t* count) const {
  int containingType = ptrInfo.typeId;
  size_t containingTypeCount{0};
  size_t internalOffset{0};

  // First, retrieve the containing type
  TypeArtStatus status = getContainingTypeInfo(addr, basePtr, ptrInfo, &containingTypeCount, &internalOffset);
  if (status != TYPEART_OK) {
    return status;
  }

  // Check for exact address match
  if (internalOffset == 0) {
    *type  = containingType;
    *count = containingTypeCount;
    return TYPEART_OK;
  }

  if (typeDB.isBuiltinType(containingType)) {
    // Address points to the middle of a builtin type
    return TYPEART_BAD_ALIGNMENT;
  }

  // Resolve struct recursively
  const auto* structInfo = typeDB.getStructInfo(containingType);
  if (structInfo != nullptr) {
    const void* containingTypeAddr = addByteOffset(addr, -std::ptrdiff_t(internalOffset));
    return getTypeInfoInternal(containingTypeAddr, internalOffset, *structInfo, type, count);
  }
  return TYPEART_INVALID_ID;
}

TypeResolution::TypeArtStatus TypeResolution::getContainingTypeInfo(const void* addr, const void* basePtr,
                                                                    const PointerInfo& ptrInfo, size_t* count,
                                                                    size_t* offset) const {
  const auto& basePtrInfo = ptrInfo;
  size_t typeSize         = typeDB.getTypeSize(basePtrInfo.typeId);

  // Check for exact match -> no further checks and offsets calculations needed
  if (basePtr == addr) {
    *count  = ptrInfo.count;
    *offset = 0;
    return TYPEART_OK;
  }

  // The address points inside a known array
  const void* blockEnd = addByteOffset(basePtr, basePtrInfo.count * typeSize);

  // Ensure that the given address is in bounds and points to the start of an element
  if (addr >= blockEnd) {
    const std::ptrdiff_t offset2base = static_cast<const uint8_t*>(addr) - static_cast<const uint8_t*>(basePtr);
    const auto oob_index             = (offset2base / typeSize) - basePtrInfo.count + 1;
    LOG_WARNING("Out of bounds for the lookup: (" << typeart::RuntimeSystem::get().toString(addr, basePtrInfo)
                                                  << ") #Elements too far: " << oob_index);
    return TYPEART_UNKNOWN_ADDRESS;
  }

  assert(addr >= basePtr && "Error in base address computation");
  size_t addrDif = reinterpret_cast<size_t>(addr) - reinterpret_cast<size_t>(basePtr);

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
  if (typeDB.isReservedType(ptrInfo.typeId)) {
    *type = static_cast<BuiltinType>(ptrInfo.typeId);
    return TYPEART_OK;
  }
  return TYPEART_WRONG_KIND;
}

TypeResolution::TypeArtStatus TypeResolution::getStructInfo(int id, const StructTypeInfo** structInfo) const {
  // Requested ID must correspond to a struct
  if (!typeDB.isStructType(id)) {
    return TYPEART_WRONG_KIND;
  }

  const auto* result = typeDB.getStructInfo(id);

  if (result != nullptr) {
    *structInfo = result;
    return TYPEART_OK;
  }
  return TYPEART_INVALID_ID;
}

const TypeDB& TypeResolution::db() const {
  return typeDB;
}

}  // namespace typeart
