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

#ifndef TYPEART_RUNTIME_H
#define TYPEART_RUNTIME_H

#include "../meta/Database.hpp"
#include "AccessCounter.hpp"

#include <cstddef>
#include <result.hpp>
#include <string>

namespace typeart {

enum class Status {
  OK,
  UNKNOWN_ADDRESS,
  BAD_ALIGNMENT,
  OFFSET_OUT_OF_RANGE,
  BAD_OFFSET,
  WRONG_KIND,
  INVALID_ALLOC_ID,
  INVALID_META_ID,
  UNSUPPORTED_TYPE
};

std::ostream& operator<<(std::ostream& os, const Status& status);

using pointer     = meta::pointer;
using byte_size   = meta::byte_size;
using byte_offset = meta::byte_offset;

class PointerInfo final {
  // The base address for the subrange this pointer info represents.
  meta::pointer base_addr = meta::pointer{nullptr};

  // The meta info for the whole allocation, **not** just the subrange beginning at base_addr.
  const meta::Allocation* allocation = nullptr;

  // The exact type of the element within the allocation which base_addr points to.
  const meta::di::Type* type = nullptr;

  // The element count of the allocation w.r.t. the address this instance was
  // queried with. (e.g. the element count of a subarray)
  size_t count = 0;

  // Type containing information on the position of a pointer within an allocation.
  //
  // Given an array of some type, the elements of the original PointerInfo (this) and
  // the subrange describe the following for a given address (addr):
  //
  //    this.count               subrange.count = this.count - base_idx
  //    |    subrange.offset     |
  //    |                  | /-----------------------\
  //    |                  |>/---\
  // /-----------------------------------------------\
  // |       |       |       |       |       |       |
  // ^                       ^   ^
  // this.base_addr          |   |
  //                         |   addr
  //                         subrange.base_addr
  //                         subrange.base_idx
  struct Subrange final {
    // The address to the beginning of the subrange.
    pointer base_addr;

    // The first index that is partially or fully within the subrange.
    size_t base_idx;

    // The offset of the pointer within the element at index lower_bound.
    byte_offset offset;

    // The element count of the subrange.
    size_t count;
  };

  friend std::ostream& operator<<(std::ostream& os, const PointerInfo::Subrange& subrange);

 public:
  PointerInfo() = default;
  PointerInfo(pointer base_addr, const meta::Allocation& allocation, const meta::di::Type& type, size_t count);

  static cpp::result<PointerInfo, Status> get(pointer addr);
  static inline cpp::result<PointerInfo, Status> get(const void* addr) {
    return get(pointer{addr});
  }

  inline pointer getBaseAddr() const {
    return base_addr;
  }
  inline const meta::Allocation& getAllocation() const {
    return *allocation;
  }
  inline const meta::di::Type& getType() const {
    return *type;
  }
  inline size_t getCount() const {
    return count;
  }

  // Returns true if the pointer p is within the address range of the
  // allocation described by this pointer info instance.
  bool contains(pointer p) const;

  PointerInfo stripTypedefsAndQualifiers() const&;
  PointerInfo stripTypedefsAndQualifiers() &&;

  // If the type of this instance is a structure type and it has a
  // member with offset 0, a pointer info instance describing the
  // first member is returned.
  // If the type is an array type, a pointer info with it's base type
  // and element count is returned.
  // If the type is neither, an Status::WRONG_KIND is returned.
  // Note: typedefs, const, ... are **not** resolved by this function.
  cpp::result<PointerInfo, Status> resolveStructureOrArrayType() const;

  // Strips typedefs and qualifiers and resolves array types.
  PointerInfo resolveAllArrayTypes() const;

  // Strips typedefs and qualifiers and resolves structure and array
  // types until the innermost type is reached.
  PointerInfo resolveToInnermostType() const;

  cpp::result<PointerInfo, Status> findMember(byte_offset offset) const;

 private:
  cpp::result<Subrange, Status> getSubrange(pointer addr) const;
  cpp::result<PointerInfo, Status> resolveSubtype(pointer addr) const;
};

std::ostream& operator<<(std::ostream& os, const PointerInfo& pointer_info);

// Type containing information on the position of a pointer within a struct type.
struct StructMemberInfo final {
  // The address of the member.
  pointer base_addr;

  // Debug info of the member.
  const meta::di::Member* member;

  // The offset of the pointer within the member of the struct.
  byte_offset offset;

 public:
  static cpp::result<StructMemberInfo, Status> get(pointer base_addr, byte_offset offset,
                                                   const meta::di::StructureType& type);

  static cpp::result<StructMemberInfo, Status> get(const PointerInfo& pointer_info, byte_offset offset);

  std::optional<PointerInfo> intoPointerInfo(const PointerInfo& original) const;
};

// Type containing information on the position of a pointer within an array type.
struct ArrayElementInfo final {
  // Base address of the first element.
  pointer base_addr;

  // Debug info for the array.
  const meta::di::ArrayType* type;

  // Index of the elment within which addr is.
  size_t index;

  // The offset of the pointer within the element of the array.
  byte_offset offset;

 public:
  static cpp::result<ArrayElementInfo, Status> get(pointer base_addr, byte_offset offset,
                                                   const meta::di::ArrayType& type);

  std::optional<PointerInfo> intoPointerInfo(const PointerInfo& original) const;
};

}  // namespace typeart

#endif  // TYPEART_RUNTIME_H
