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

#include "../db/Database.hpp"
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
  UNSUPPORTED_TYPE,
  ERROR
};

std::ostream& operator<<(std::ostream& os, const Status& status);

}  // namespace typeart

namespace typeart::runtime {

class pointer {
  const void* value;

 public:
  explicit pointer(const void* value) : value(value) {
  }

  const void* get() const {
    return value;
  }

  operator const void*() const {
    return value;
  }
};

inline bool operator==(pointer lhs, pointer rhs) {
  return lhs.get() == rhs.get();
}

inline pointer operator+(pointer p, ptrdiff_t offset) {
  return pointer{static_cast<const int8_t*>(p.get()) + offset};
}

class byte_size {
  using value_type = size_t;

  value_type _value;

  byte_size(value_type value) : _value(value) {
  }

 public:
  static byte_size from_bits(value_type bits) {
    assert(bits % 8 == 0);
    return {bits / 8};
  }

  static byte_size from_bytes(value_type bytes) {
    return {bytes};
  }

  value_type value() const {
    return _value;
  }

  value_type as_bits() const {
    return _value * 8;
  }
};

inline std::ostream& operator<<(std::ostream& os, const byte_size& value) {
  os << value.value() << "B";
  return os;
}

inline byte_size operator*(size_t lhs, byte_size rhs) {
  return byte_size::from_bytes(lhs * rhs.value());
}

inline pointer operator+(pointer p, byte_size offset) {
  return pointer{static_cast<const int8_t*>(p.get()) + offset.value()};
}

class byte_offset {
  using value_type = ssize_t;

  value_type _value;

  byte_offset(value_type value) : _value(value) {
  }

 public:
  static byte_offset zero;

  static byte_offset from_bits(value_type bits) {
    assert(bits % 8 == 0);
    return {bits / 8};
  }

  static byte_offset from_bytes(value_type bytes) {
    return {bytes};
  }

  value_type value() const {
    return _value;
  }

  value_type as_bits() const {
    return _value * 8;
  }
};

inline std::ostream& operator<<(std::ostream& os, const byte_offset& value) {
  os << value.value() << "B";
  return os;
}

inline bool operator==(byte_offset lhs, byte_offset rhs) {
  return lhs.value() == rhs.value();
}

inline bool operator>=(byte_offset lhs, byte_offset rhs) {
  return lhs.value() >= rhs.value();
}

inline byte_offset operator-(pointer lhs, pointer rhs) {
  return byte_offset::from_bytes(static_cast<const int8_t*>(lhs.get()) - static_cast<const int8_t*>(rhs.get()));
}

inline byte_offset operator-(byte_offset lhs, byte_offset rhs) {
  return byte_offset::from_bytes(lhs.value() - rhs.value());
}

inline pointer operator+(pointer p, byte_offset offset) {
  return pointer{static_cast<const int8_t*>(p.get()) + offset.value()};
}

inline pointer operator-(pointer p, byte_offset offset) {
  return pointer{static_cast<const int8_t*>(p.get()) - offset.value()};
}

inline byte_offset operator%(byte_offset lhs, byte_size rhs) {
  return byte_offset::from_bytes(lhs.value() % rhs.value());
}

inline ssize_t operator/(byte_offset lhs, byte_size rhs) {
  return lhs.value() / rhs.value();
}

class PointerInfo final {
  // The base address of the whole allocation.
  pointer base_addr = pointer{nullptr};

  // The meta info for this allocation.
  const meta::Allocation* allocation = nullptr;

  // The exact type of the element within the allocation which the pointer points to.
  const meta::di::Type* type = nullptr;

  // The element count of the allocation w.r.t. the address this instance was
  // queried with. (e.g. the element count of a subarray)
  size_t count = 0;

  // Type containing information on the position of a pointer within an allocation.
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

  // Returns this pointer info with it's type canonicalized.
  PointerInfo getCanonicalized() const&;

  // Returns this pointer info with it's type canonicalized.
  PointerInfo getCanonicalized() &&;

  // If the type of this instance is a structure type, a pointer info
  // instance describing the first member is returned.
  // If the type is an array type, a pointer info with it's base type
  // and element count is returned.
  // If the type is neither, an Status::WRONG_KIND is returned.
  // Note: typedefs, const, ... are **not** resolved by this function.
  cpp::result<PointerInfo, Status> resolveStructureOrArrayType() const;

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
  const meta::di::DerivedType* member;

  // The offset of the pointer within the member of the struct.
  byte_offset offset;

 public:
  static cpp::result<StructMemberInfo, Status> get(pointer base_addr, byte_offset offset,
                                                   const meta::di::StructureType& type);

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

struct ScopeGuard final {
  ScopeGuard();
  ~ScopeGuard();

  ScopeGuard(const ScopeGuard&) = delete;
  ScopeGuard(ScopeGuard&&)      = delete;

  ScopeGuard& operator=(const ScopeGuard&) = delete;
  ScopeGuard& operator=(ScopeGuard&&) = delete;

  bool shouldTrack() const;
};

Recorder& getRecorder();
const meta::Meta* getMeta(meta_id_t meta_id);
const AllocationInfo* getAllocationInfo(alloc_id_t alloc_id);

}  // namespace typeart::runtime

#endif  // TYPEART_RUNTIME_H
