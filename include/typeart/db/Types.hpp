// TypeART library
//
// Copyright (c) 2017-2021 TypeART Authors
// Distributed under the BSD 3-Clause license.
// (See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/BSD-3-Clause)
//
// Project home: https://github.com/tudasc/TypeART
//
// SPDX-License-Identifier: BSD-3-Clause
//

#pragma once

#include "../support/System.hpp"
#include "Types.h"

#include <cassert>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <ostream>
#include <type_traits>
#include <variant>
#include <vector>

namespace llvm {
class Metadata;
}

#define META_CLASS(BASE, KIND, REF_COUNT)                                                    \
 public:                                                                                     \
  inline KIND()                                                                              \
      : BASE(meta_id_t::invalid, Kind::KIND,                                                 \
             std::vector<Ref<Meta>>(REF_COUNT, Ref<Meta>{meta_id_t::invalid, Kind::KIND})) { \
  }                                                                                          \
  constexpr static Kind META_KIND = Kind::KIND;                                              \
  static inline bool classof(Kind kind) {                                                    \
    switch (kind) {                                                                          \
      case Kind::KIND:                                                                       \
        return true;                                                                         \
      default:                                                                               \
        return false;                                                                        \
    }                                                                                        \
  }

#define META_MEMBER(TYPE, NAME)           \
 private:                                 \
  TYPE _##NAME;                           \
                                          \
 public:                                  \
  inline const TYPE& get_##NAME() const { \
    return _##NAME;                       \
  }                                       \
  inline TYPE& get_##NAME() {             \
    return _##NAME;                       \
  }                                       \
  inline void set_##NAME(TYPE value) {    \
    _##NAME = std::move(value);           \
  }

#define META_REF(INDEX, TYPE, NAME)                 \
 public:                                            \
  inline TYPE* get_##NAME() const {                 \
    return dyn_cast<TYPE>(_refs[INDEX].get_meta()); \
  }                                                 \
  inline Ref<TYPE> get_##NAME##_raw() const {       \
    return dyn_cast<TYPE>(_refs[INDEX]);            \
  }                                                 \
  inline void set_##NAME##_raw(Ref<TYPE> new_ref) { \
    _refs[INDEX] = new_ref;                         \
  }

#define META_TUPLE(INDEX, TYPE, NAME)                                                         \
 public:                                                                                      \
  inline Ref<TYPE> get_##NAME(size_t idx) const {                                             \
    auto meta = dyn_cast<Tuple>(_refs[INDEX]).get_meta();                                     \
    if (!meta) {                                                                              \
      fprintf(stderr, "Cannot get tuple element from Meta instance with unresolved Refs!\n"); \
      abort;                                                                                  \
    }                                                                                         \
    return dyn_cast<TYPE>(meta->get_refs()[idx]);                                             \
  }                                                                                           \
  inline Tuple* get_##NAME##s() const {                                                       \
    return dyn_cast<Tuple>(_refs[INDEX].get_meta());                                          \
  }                                                                                           \
  inline Ref<Meta> get_##NAME##s_raw() {                                                      \
    return _refs[INDEX];                                                                      \
  }                                                                                           \
  inline void set_##NAME##s_raw(Ref<Tuple> refs) {                                            \
    _refs[INDEX] = refs;                                                                      \
  }

namespace typeart {

class Database;

struct type_id_t {
  using value_type = type_id_value;

  static const type_id_t unknown_type;
  static const type_id_t invalid;

 private:
  value_type _value = TYPEART_UNKNOWN_TYPE;

 public:
  type_id_t() = default;

  type_id_t(value_type value) : _value(value) {
  }

  template <class T>
  type_id_t(T t) = delete;

  value_type value() const {
    return _value;
  }
};

inline bool operator==(const type_id_t& lhs, const type_id_t& rhs) {
  return lhs.value() == rhs.value();
}

inline bool operator!=(const type_id_t& lhs, const type_id_t& rhs) {
  return !(lhs == rhs);
}

inline bool operator<(const type_id_t& lhs, const type_id_t& rhs) {
  return lhs.value() < rhs.value();
}

std::ostream& operator<<(std::ostream& os, const type_id_t& alloc_id);

struct alloc_id_t {
  using value_type = alloc_id_value;

  static const alloc_id_t invalid;

 private:
  value_type _value = 0;

 public:
  alloc_id_t() = default;

  alloc_id_t(value_type value) : _value(value) {
  }

  template <class T>
  alloc_id_t(T t) = delete;

  value_type value() const {
    return _value;
  }
};

inline bool operator==(const alloc_id_t& lhs, const alloc_id_t& rhs) {
  return lhs.value() == rhs.value();
}

inline bool operator!=(const alloc_id_t& lhs, const alloc_id_t& rhs) {
  return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const alloc_id_t& alloc_id);

struct meta_id_t {
  using value_type = meta_id_value;

  static const meta_id_t invalid;

 private:
  value_type _value = 0;

 public:
  meta_id_t() = default;

  meta_id_t(value_type value) : _value(value) {
  }

  template <class T>
  meta_id_t(T t) = delete;

  value_type value() const {
    return _value;
  }
};

inline bool operator==(const meta_id_t& lhs, const meta_id_t& rhs) {
  return lhs.value() == rhs.value();
}

inline bool operator!=(const meta_id_t& lhs, const meta_id_t& rhs) {
  return !(lhs == rhs);
}

namespace meta {

enum class Kind {
  Unknown,
  CompileUnit,
  File,
  Subprogram,
  Namespace,
  BasicType,
  CompositeType,
  DerivedType,
  SubroutineType,
  Location,
  LocalVariable,
  GlobalVariable,
  HeapAllocation,
  StackAllocation,
  GlobalAllocation,
  Subrange,
  LexicalBlock,
  LexicalBlockFile,
  Enumerator,
  String,
  Tuple,
};

std::ostream& operator<<(std::ostream& os, const Kind& kind);

class Meta;

namespace detail {
Meta* resolve_meta_id(meta_id_t id, Database& db);

template <typename T, typename = int>
struct is_leaf_class : std::false_type {};

template <typename T>
struct is_leaf_class<T, decltype((void)T::META_KIND, 0)> : std::true_type {};
}  // namespace detail

template <class MetaClass>
class Ref {
  static_assert(std::is_base_of_v<Meta, MetaClass>);

  Kind kind;
  bool _is_weak = false;
  std::variant<meta_id_t, MetaClass*> ref;

 public:
  Ref() : kind(Kind::Unknown), ref(meta_id_t::invalid) {
  }

  Ref(meta_id_t id, Kind kind, bool is_weak = false) : kind(kind), _is_weak(is_weak), ref(id) {
  }

  template <class _MetaClass = MetaClass, class = std::enable_if_t<detail::is_leaf_class<_MetaClass>::value>>
  Ref(MetaClass* ptr) : Ref(ptr, MetaClass::META_KIND) {
  }

  Ref(MetaClass* ptr, Kind kind, bool is_weak = false) : kind(kind), _is_weak(is_weak) {
    if (ptr != nullptr) {
      ref = ptr;
    } else {
      ref = meta_id_t::invalid;
    }
  }

  template <class OtherMeta, class = std::enable_if_t<std::is_base_of_v<MetaClass, OtherMeta>>>
  Ref(const Ref<OtherMeta>& other) : kind(other.get_kind()), _is_weak(other.is_weak()) {
    if (auto ptr = other.get_meta(); ptr != nullptr) {
      ref = ptr;
    } else {
      ref = other.get_id();
    }
  }

  bool is_weak() const {
    return _is_weak;
  }

  void set_weak(bool weak) {
    _is_weak = weak;
  }

  Kind get_kind() const {
    return kind;
  }

  void set_id(meta_id_t new_id) {
    if (ref.index() == 0) {
      ref = new_id;
    } else {
      std::get<1>(ref)->set_id(new_id);
    }
  }

  meta_id_t get_id() const {
    if (ref.index() == 0) {
      return std::get<0>(ref);
    } else {
      auto ptr = std::get<1>(ref);
      return ptr != nullptr ? ptr->get_id() : meta_id_t::invalid;
    }
  }

  void set_meta(MetaClass* meta) {
    if (meta != nullptr) {
      kind = meta->get_kind();
      ref  = meta;
    } else {
      ref = meta_id_t::invalid;
    }
  }

  MetaClass* get_meta() const {
    if (ref.index() == 0) {
      return nullptr;
    } else {
      return std::get<1>(ref);
    }
  }

  inline MetaClass* operator->() const {
    return get_meta();
  }

  inline MetaClass& operator*() const {
    return *get_meta();
  }

  inline bool is_valid() const {
    return !(get_id() == meta_id_t::invalid && get_kind() == Kind::Unknown);
  }
};

template <class MetaClass>
inline bool operator==(const Ref<MetaClass>& lhs, const Ref<MetaClass>& rhs) {
  return lhs.get_kind() == rhs.get_kind() && (lhs.is_weak() || rhs.is_weak() || lhs.get_id() == rhs.get_id());
}

class Meta {
  Kind _kind;
  META_MEMBER(meta_id_t, id)

 protected:
  std::vector<Ref<Meta>> _refs;

  inline Meta(meta_id_t id, Kind kind, std::vector<Ref<Meta>> refs) : _kind(kind), _id(id), _refs(std::move(refs)) {
  }

 public:
  static inline bool classof(Kind kind) {
    return true;
  }

  virtual ~Meta();

  inline Kind get_kind() const {
    return _kind;
  }

  inline const std::vector<Ref<Meta>>& get_refs() const {
    return _refs;
  }

  inline std::vector<Ref<Meta>>& get_refs() {
    return _refs;
  }
};

template <class To, class = std::enable_if_t<std::is_base_of_v<Meta, To>>>
To* dyn_cast(Meta* meta) {
  if (meta == nullptr) {
    return nullptr;
  }
  return To::classof(meta->get_kind()) ? static_cast<To*>(meta) : nullptr;
}

template <class To, class = std::enable_if_t<std::is_base_of_v<Meta, To>>>
const To* dyn_cast(const Meta* meta) {
  if (meta == nullptr) {
    return nullptr;
  }
  return To::classof(meta->get_kind()) ? static_cast<To*>(meta) : nullptr;
}

template <class To, class = std::enable_if_t<std::is_base_of_v<Meta, To>>>
Ref<To> dyn_cast(const Ref<Meta>& ref) {
  if (auto ptr = ref.get_meta(); ptr != nullptr) {
    if (auto casted = dyn_cast<To>(ptr); casted != nullptr) {
      return {casted, ref.get_kind(), ref.is_weak()};
    }
  } else {
    if (To::classof(ref.get_kind())) {
      return {ref.get_id(), ref.get_kind(), ref.is_weak()};
    }
  }
  return {};
}

class String final : public Meta {
  META_CLASS(Meta, String, 0)
  META_MEMBER(std::string, data)

 public:
  inline String(meta_id_t id, std::string data) : Meta(id, Kind::String, {}), _data(std::move(data)) {
  }

  virtual ~String();
};

inline bool operator==(const String& lhs, const String& rhs) {
  return lhs.get_refs() == rhs.get_refs() && lhs.get_data() == rhs.get_data();
}

class Tuple final : public Meta {
  META_CLASS(Meta, Tuple, 0)

 public:
  inline Tuple(meta_id_t id, std::vector<Ref<Meta>> refs) : Meta(id, Kind::Tuple, std::move(refs)) {
  }

  virtual ~Tuple();
};

inline bool operator==(const Tuple& lhs, const Tuple& rhs) {
  return lhs.get_refs() == rhs.get_refs();
}

class Node : public Meta {
 protected:
  inline Node(meta_id_t id, Kind kind, std::vector<Ref<Meta>> refs) : Meta(id, kind, std::move(refs)) {
  }

 public:
  static inline bool classof(Kind kind) {
    switch (kind) {
      case Kind::CompileUnit:
      case Kind::File:
      case Kind::Subprogram:
      case Kind::Namespace:
      case Kind::BasicType:
      case Kind::CompositeType:
      case Kind::DerivedType:
      case Kind::SubroutineType:
      case Kind::Location:
      case Kind::LocalVariable:
      case Kind::GlobalVariable:
      case Kind::Subrange:
      case Kind::LexicalBlock:
      case Kind::LexicalBlockFile:
      case Kind::Enumerator:
        return true;
      default:
        return false;
    }
  }

  virtual ~Node();
};

namespace di {

class Node : public meta::Node {
 protected:
  inline Node(meta_id_t id, Kind kind, std::vector<Ref<Meta>> refs) : meta::Node(id, kind, std::move(refs)) {
  }

 public:
  static inline bool classof(Kind kind) {
    switch (kind) {
      case Kind::CompileUnit:
      case Kind::File:
      case Kind::Subprogram:
      case Kind::Namespace:
      case Kind::BasicType:
      case Kind::CompositeType:
      case Kind::DerivedType:
      case Kind::SubroutineType:
      case Kind::LocalVariable:
      case Kind::GlobalVariable:
      case Kind::Subrange:
      case Kind::LexicalBlock:
      case Kind::LexicalBlockFile:
      case Kind::Enumerator:
        return true;
      default:
        return false;
    }
  }

  virtual ~Node();
};

class Subrange final : public di::Node {
  META_CLASS(di::Node, Subrange, 0)
  META_MEMBER(std::optional<size_t>, count)
  META_MEMBER(size_t, lower_bound)

 public:
  virtual ~Subrange();
};

inline bool operator==(const Subrange& lhs, const Subrange& rhs) {
  return lhs.get_refs() == rhs.get_refs() && lhs.get_count() == rhs.get_count() &&
         lhs.get_lower_bound() == rhs.get_lower_bound();
}

class Enumerator final : public di::Node {
  META_CLASS(di::Node, Enumerator, 1)
  META_MEMBER(ssize_t, value)
  META_MEMBER(bool, is_unsigned)
  META_REF(0, meta::String, name)

 public:
  virtual ~Enumerator();
};

inline bool operator==(const Enumerator& lhs, const Enumerator& rhs) {
  return lhs.get_refs() == rhs.get_refs() && lhs.get_value() == rhs.get_value() &&
         lhs.get_is_unsigned() == rhs.get_is_unsigned();
}

class Scope : public di::Node {
 protected:
  inline Scope(meta_id_t id, Kind kind, std::vector<Ref<Meta>> refs) : di::Node(id, kind, std::move(refs)) {
  }

 public:
  static inline bool classof(Kind kind) {
    switch (kind) {
      case Kind::CompileUnit:
      case Kind::File:
      case Kind::Subprogram:
      case Kind::Namespace:
      case Kind::BasicType:
      case Kind::CompositeType:
      case Kind::DerivedType:
      case Kind::SubroutineType:
      case Kind::LexicalBlock:
      case Kind::LexicalBlockFile:
        return true;
      default:
        return false;
    }
  }

  virtual ~Scope();
};

enum class Language {
  C,
  C89,
  C99,
  C11,
  Cpp,
  Cpp03,
  Cpp11,
  Cpp14,
};
class File final : public di::Scope {
  META_CLASS(di::Scope, File, 2)
  META_REF(0, meta::String, filename)
  META_REF(1, meta::String, directory)

 public:
  virtual ~File();
};

inline bool operator==(const File& lhs, const File& rhs) {
  return lhs.get_refs() == rhs.get_refs();
}

class CompileUnit final : public di::Scope {
  META_CLASS(di::Scope, CompileUnit, 2)
  META_MEMBER(Language, language)
  META_MEMBER(bool, is_optimized)
  META_MEMBER(size_t, runtime_version)
  META_REF(0, di::File, file)
  META_REF(1, meta::String, producer)

 public:
  virtual ~CompileUnit();
};

inline bool operator==(const CompileUnit& lhs, const CompileUnit& rhs) {
  return lhs.get_refs() == rhs.get_refs() && lhs.get_language() == rhs.get_language() &&
         lhs.get_is_optimized() == rhs.get_is_optimized() && lhs.get_runtime_version() == rhs.get_runtime_version();
}

class Namespace final : public di::Scope {
  META_CLASS(di::Scope, Namespace, 2)
  META_REF(0, String, name)
  META_REF(1, di::Scope, scope)

 public:
  virtual ~Namespace();
};

inline bool operator==(const Namespace& lhs, const Namespace& rhs) {
  return lhs.get_refs() == rhs.get_refs();
}

class Type : public di::Scope {
 protected:
  inline Type(meta_id_t id, Kind kind, std::vector<Ref<Meta>> refs) : di::Scope(id, kind, std::move(refs)) {
  }

 public:
  static inline bool classof(Kind kind) {
    switch (kind) {
      case Kind::BasicType:
      case Kind::CompositeType:
      case Kind::DerivedType:
      case Kind::SubroutineType:
        return true;
      default:
        return false;
    }
  }

  virtual ~Type();
};

enum class Encoding {
  Unspecified,
  Address,
  Boolean,
  Float,
  Signed,
  Unsigned,
  SignedChar,
  UnsignedChar,
};
class BasicType final : public di::Type {
  META_CLASS(di::Type, BasicType, 1)
  META_MEMBER(Encoding, encoding)
  META_MEMBER(size_t, size_in_bits)
  META_REF(0, String, name)

 public:
  virtual ~BasicType();
};

inline bool operator==(const BasicType& lhs, const BasicType& rhs) {
  return lhs.get_refs() == rhs.get_refs() && lhs.get_size_in_bits() == rhs.get_size_in_bits() &&
         lhs.get_encoding() == rhs.get_encoding();
}

enum class CompositeKind {
  Structure,
  Class,
  Union,
};
class CompositeType final : public di::Type {
  META_CLASS(di::Type, CompositeType, 5)
  META_MEMBER(size_t, line)
  META_MEMBER(size_t, size_in_bits)
  META_REF(0, String, name)
  META_REF(1, String, identifier)
  META_REF(2, di::File, file)
  META_REF(3, di::Scope, scope)
  META_TUPLE(4, di::Node, element)

 public:
  virtual ~CompositeType();
};

inline bool operator==(const CompositeType& lhs, const CompositeType& rhs) {
  return lhs.get_refs() == rhs.get_refs() && lhs.get_size_in_bits() == rhs.get_size_in_bits() &&
         lhs.get_line() == rhs.get_line();
}

enum class DerivedKind {
  Member,
  Typedef,
  Pointer,
  Reference,
  RvalueReference,
  Const,
  Inheritance,
  Restrict,
  Volatile,
  PtrToMemberType,
};
class DerivedType final : public di::Type {
  META_CLASS(di::Type, DerivedType, 4)
  META_MEMBER(DerivedKind, tag)
  META_MEMBER(size_t, line)
  META_MEMBER(size_t, offset_in_bits)
  META_MEMBER(size_t, size_in_bits)
  META_REF(0, String, name)
  META_REF(1, di::File, file)
  META_REF(2, di::Scope, scope)
  META_REF(3, di::Type, base_type)

 public:
  virtual ~DerivedType();
};

inline bool operator==(const DerivedType& lhs, const DerivedType& rhs) {
  return lhs.get_refs() == rhs.get_refs() && lhs.get_size_in_bits() == rhs.get_size_in_bits() &&
         lhs.get_kind() == rhs.get_kind() && lhs.get_line() == rhs.get_line() &&
         lhs.get_offset_in_bits() == rhs.get_offset_in_bits();
}

class SubroutineType final : public di::Type {
  META_CLASS(di::Type, SubroutineType, 2)
  META_REF(0, di::Type, return_type)
  META_TUPLE(1, di::Type, argument_type)

 public:
  virtual ~SubroutineType();
};

inline bool operator==(const SubroutineType& lhs, const SubroutineType& rhs) {
  return lhs.get_refs() == rhs.get_refs();
}

class LocalScope : public di::Scope {
 protected:
  inline LocalScope(meta_id_t id, Kind kind, std::vector<Ref<Meta>> refs) : di::Scope(id, kind, std::move(refs)) {
  }

 public:
  static inline bool classof(Kind kind) {
    switch (kind) {
      case Kind::Subprogram:
      case Kind::LexicalBlock:
      case Kind::LexicalBlockFile:
        return true;
      default:
        return false;
    }
  }

  virtual ~LocalScope();
};

class LexicalBlockBase : public di::LocalScope {
 protected:
  inline LexicalBlockBase(meta_id_t id, Kind kind, std::vector<Ref<Meta>> refs)
      : di::LocalScope(id, kind, std::move(refs)) {
  }

 public:
  static inline bool classof(Kind kind) {
    switch (kind) {
      case Kind::LexicalBlock:
      case Kind::LexicalBlockFile:
        return true;
      default:
        return false;
    }
  }

  virtual ~LexicalBlockBase();
};

class LexicalBlock final : public di::LexicalBlockBase {
  META_CLASS(di::LexicalBlockBase, LexicalBlock, 2)
  META_MEMBER(size_t, line)
  META_MEMBER(size_t, column)
  META_REF(0, di::Scope, scope)
  META_REF(1, di::File, file)

 public:
  virtual ~LexicalBlock();
};

inline bool operator==(const LexicalBlock& lhs, const LexicalBlock& rhs) {
  return lhs.get_refs() == rhs.get_refs() && lhs.get_line() == rhs.get_line() && lhs.get_column() == rhs.get_column();
}

class LexicalBlockFile final : public di::LexicalBlockBase {
  META_CLASS(di::LexicalBlockBase, LexicalBlockFile, 2)
  META_MEMBER(size_t, discriminator)
  META_REF(0, di::Scope, scope)
  META_REF(1, di::File, file)

 public:
  virtual ~LexicalBlockFile();
};

inline bool operator==(const LexicalBlockFile& lhs, const LexicalBlockFile& rhs) {
  return lhs.get_refs() == rhs.get_refs() && lhs.get_discriminator() == rhs.get_discriminator();
}

class Subprogram final : public di::LocalScope {
  META_CLASS(di::LocalScope, Subprogram, 5)
  META_MEMBER(size_t, line)
  META_REF(0, String, name)
  META_REF(1, String, linkage_name)
  META_REF(2, di::File, file)
  META_REF(3, di::Scope, scope)
  META_REF(4, di::SubroutineType, type)

 public:
  virtual ~Subprogram();
};

inline bool operator==(const Subprogram& lhs, const Subprogram& rhs) {
  return lhs.get_refs() == rhs.get_refs() && lhs.get_line() == rhs.get_line();
}

class Location final : public meta::Node {
  META_CLASS(meta::Node, Location, 1)
  META_MEMBER(size_t, line)
  META_MEMBER(size_t, column)
  META_REF(0, di::LocalScope, scope)

 public:
  virtual ~Location();
};

inline bool operator==(const Location& lhs, const Location& rhs) {
  return lhs.get_refs() == rhs.get_refs() && lhs.get_line() == rhs.get_line() && lhs.get_column() == rhs.get_column();
}

class Variable : public di::Node {
 protected:
  inline Variable(meta_id_t id, Kind kind, std::vector<Ref<Meta>> refs) : di::Node(id, kind, std::move(refs)) {
  }

  virtual ~Variable();

 public:
  static inline bool classof(Kind kind) {
    switch (kind) {
      case Kind::LocalVariable:
      case Kind::GlobalVariable:
        return true;
      default:
        return false;
    }
  }
};

class LocalVariable final : public Variable {
  META_CLASS(Variable, LocalVariable, 5)
  META_MEMBER(size_t, line)
  META_REF(0, String, name)
  META_REF(1, String, linkage_name)
  META_REF(2, Scope, scope)
  META_REF(3, File, file)
  META_REF(4, Type, type)

 public:
  virtual ~LocalVariable();
};

inline bool operator==(const LocalVariable& lhs, const LocalVariable& rhs) {
  return lhs.get_refs() == rhs.get_refs() && lhs.get_line() == rhs.get_line();
}

class GlobalVariable final : public Variable {
  META_CLASS(Variable, GlobalVariable, 5)
  META_MEMBER(size_t, line)
  META_MEMBER(bool, is_local)
  META_MEMBER(bool, is_definition)
  META_REF(0, String, name)
  META_REF(1, String, linkage_name)
  META_REF(2, Scope, scope)
  META_REF(3, File, file)
  META_REF(4, Type, type)

 public:
  virtual ~GlobalVariable();
};

inline bool operator==(const GlobalVariable& lhs, const GlobalVariable& rhs) {
  return lhs.get_refs() == rhs.get_refs() && lhs.get_line() == rhs.get_line() && lhs.get_type() == rhs.get_type() &&
         lhs.get_is_local() == rhs.get_is_local() && lhs.get_is_definition() == rhs.get_is_definition();
}

}  // namespace di

class Allocation : public Meta {
 public:
  inline Allocation(meta_id_t id, Kind kind, std::vector<Ref<Meta>> refs) : Meta(id, kind, std::move(refs)) {
  }

  virtual ~Allocation();

  static inline bool classof(Kind kind) {
    switch (kind) {
      case Kind::StackAllocation:
      case Kind::HeapAllocation:
      case Kind::GlobalAllocation:
        return true;
      default:
        return false;
    }
  }
};

class StackAllocation final : public Allocation {
  META_CLASS(Allocation, StackAllocation, 2)
  META_REF(0, di::LocalVariable, local_variable)
  META_REF(1, di::Location, location)

 public:
  virtual ~StackAllocation();
};

inline bool operator==(const StackAllocation& lhs, const StackAllocation& rhs) {
  return lhs.get_refs() == rhs.get_refs();
}

class HeapAllocation final : public Allocation {
  META_CLASS(Allocation, HeapAllocation, 2)
  META_REF(0, di::Type, type)
  META_REF(1, di::Location, location)

 public:
  virtual ~HeapAllocation();
};

inline bool operator==(const HeapAllocation& lhs, const HeapAllocation& rhs) {
  return lhs.get_refs() == rhs.get_refs();
}

class GlobalAllocation final : public Allocation {
  META_CLASS(Allocation, GlobalAllocation, 1)
  META_REF(0, di::GlobalVariable, global_variable)

 public:
  virtual ~GlobalAllocation();
};

inline bool operator==(const GlobalAllocation& lhs, const GlobalAllocation& rhs) {
  return lhs.get_refs() == rhs.get_refs();
}

inline bool operator==(const Meta& lhs, const Meta& rhs) {
  if (lhs.get_kind() != rhs.get_kind()) {
    return false;
  }
  if (lhs.get_id() == rhs.get_id()) {
    return true;
  }
  switch (lhs.get_kind()) {
    case Kind::String:
      return static_cast<const String&>(lhs) == static_cast<const String&>(rhs);
    case Kind::Tuple:
      return static_cast<const Tuple&>(lhs) == static_cast<const Tuple&>(rhs);
    case Kind::CompileUnit:
      return static_cast<const di::CompileUnit&>(lhs) == static_cast<const di::CompileUnit&>(rhs);
    case Kind::File:
      return static_cast<const di::File&>(lhs) == static_cast<const di::File&>(rhs);
    case Kind::Subprogram:
      return static_cast<const di::Subprogram&>(lhs) == static_cast<const di::Subprogram&>(rhs);
    case Kind::Namespace:
      return static_cast<const di::Namespace&>(lhs) == static_cast<const di::Namespace&>(rhs);
    case Kind::BasicType:
      return static_cast<const di::BasicType&>(lhs) == static_cast<const di::BasicType&>(rhs);
    case Kind::CompositeType:
      return static_cast<const di::CompositeType&>(lhs) == static_cast<const di::CompositeType&>(rhs);
    case Kind::DerivedType:
      return static_cast<const di::DerivedType&>(lhs) == static_cast<const di::DerivedType&>(rhs);
    case Kind::SubroutineType:
      return static_cast<const di::SubroutineType&>(lhs) == static_cast<const di::SubroutineType&>(rhs);
    case Kind::Location:
      return static_cast<const di::Location&>(lhs) == static_cast<const di::Location&>(rhs);
    case Kind::LocalVariable:
      return static_cast<const di::LocalVariable&>(lhs) == static_cast<const di::LocalVariable&>(rhs);
    case Kind::GlobalVariable:
      return static_cast<const di::GlobalVariable&>(lhs) == static_cast<const di::GlobalVariable&>(rhs);
    case Kind::HeapAllocation:
      return static_cast<const HeapAllocation&>(lhs) == static_cast<const HeapAllocation&>(rhs);
    case Kind::StackAllocation:
      return static_cast<const StackAllocation&>(lhs) == static_cast<const StackAllocation&>(rhs);
    case Kind::GlobalAllocation:
      return static_cast<const GlobalAllocation&>(lhs) == static_cast<const GlobalAllocation&>(rhs);
    case Kind::Subrange:
      return static_cast<const di::Subrange&>(lhs) == static_cast<const di::Subrange&>(rhs);
    case Kind::LexicalBlock:
      return static_cast<const di::LexicalBlock&>(lhs) == static_cast<const di::LexicalBlock&>(rhs);
    case Kind::LexicalBlockFile:
      return static_cast<const di::LexicalBlockFile&>(lhs) == static_cast<const di::LexicalBlockFile&>(rhs);
    case Kind::Enumerator:
      return static_cast<const di::Enumerator&>(lhs) == static_cast<const di::Enumerator&>(rhs);
    default:
      abort();
  }
}

}  // namespace meta

}  // namespace typeart

namespace std {

template <>
struct hash<typeart::type_id_t> {
  std::size_t operator()(const typeart::type_id_t& type_id) const {
    return hash<typeart::type_id_t::value_type>{}(type_id.value());
  }
};

}  // namespace std
