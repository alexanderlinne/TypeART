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
#include <iosfwd>
#include <limits>
#include <memory>
#include <optional>
#include <queue>
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

#define META_REF(INDEX, TYPE, NAME)                 \
 public:                                            \
  inline const TYPE& get_##NAME() const {           \
    return *get_##NAME##_raw();                     \
  }                                                 \
  inline Ref<TYPE> get_##NAME##_raw() const {       \
    return dyn_cast<TYPE>(refs[INDEX]);             \
  }                                                 \
  inline void set_##NAME##_raw(Ref<TYPE> new_ref) { \
    refs[INDEX] = new_ref;                          \
  }

#define META_REF_OVERRIDE(INDEX, TYPE, NAME)        \
 public:                                            \
  inline const TYPE& get_##NAME() const override {  \
    return *get_##NAME##_raw();                     \
  }                                                 \
  inline Ref<TYPE> get_##NAME##_raw() const {       \
    return dyn_cast<TYPE>(refs[INDEX]);             \
  }                                                 \
  inline void set_##NAME##_raw(Ref<TYPE> new_ref) { \
    refs[INDEX] = new_ref;                          \
  }

#define META_STRING(INDEX, NAME)                      \
 public:                                              \
  inline const std::string& get_##NAME() const {      \
    return get_##NAME##_raw()->get_data();            \
  }                                                   \
  inline Ref<String> get_##NAME##_raw() const {       \
    return dyn_cast<String>(refs[INDEX]);             \
  }                                                   \
  inline void set_##NAME##_raw(Ref<String> new_ref) { \
    refs[INDEX] = new_ref;                            \
  }

#define META_INTEGER(INDEX, TYPE, NAME)                       \
 public:                                                      \
  inline TYPE get_##NAME() const {                            \
    return static_cast<TYPE>(get_##NAME##_raw()->get_data()); \
  }                                                           \
  inline Ref<Integer> get_##NAME##_raw() const {              \
    return dyn_cast<Integer>(refs[INDEX]);                    \
  }                                                           \
  inline void set_##NAME##_raw(Ref<Integer> new_ref) {        \
    refs[INDEX] = new_ref;                                    \
  }

#define META_INTEGER_OVERRIDE(INDEX, TYPE, NAME)              \
 public:                                                      \
  inline TYPE get_##NAME() const override {                   \
    return static_cast<TYPE>(get_##NAME##_raw()->get_data()); \
  }                                                           \
  inline Ref<Integer> get_##NAME##_raw() const {              \
    return dyn_cast<Integer>(refs[INDEX]);                    \
  }                                                           \
  inline void set_##NAME##_raw(Ref<Integer> new_ref) {        \
    refs[INDEX] = new_ref;                                    \
  }

#define META_TUPLE(INDEX, TYPE, NAME)                                                         \
 public:                                                                                      \
  inline Ref<TYPE> get_##NAME(size_t idx) const {                                             \
    auto meta = dyn_cast<Tuple>(refs[INDEX]).get();                                           \
    if (!meta) {                                                                              \
      fprintf(stderr, "Cannot get tuple element from Meta instance with unresolved Refs!\n"); \
      abort;                                                                                  \
    }                                                                                         \
    return dyn_cast<TYPE>(meta->get_refs()[idx]);                                             \
  }                                                                                           \
  inline const Tuple& get_##NAME##s() const {                                                 \
    auto result = dyn_cast<Tuple>(refs[INDEX].get());                                         \
    assert(result != nullptr);                                                                \
    return *result;                                                                           \
  }                                                                                           \
  inline Ref<Meta> get_##NAME##s_raw() {                                                      \
    return refs[INDEX];                                                                       \
  }                                                                                           \
  inline void set_##NAME##s_raw(Ref<Tuple> ref) {                                             \
    refs[INDEX] = ref;                                                                        \
  }

namespace typeart {

template <typename ReturnType, typename... Lambdas>
struct lambda_visitor : public Lambdas... {
  using return_type = ReturnType;

  lambda_visitor(Lambdas... lambdas) : Lambdas(lambdas)... {
  }
};

template <typename ReturnType, typename... Lambdas>
lambda_visitor<ReturnType, Lambdas...> make_lambda_visitor(Lambdas... lambdas) {
  return {lambdas...};
}

class Database;

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
  GlobalOrBuiltin,
  CompileUnit,
  File,
  Subprogram,
  Namespace,
  VoidType,
  BasicType,
  StructureType,
  UnionType,
  ArrayType,
  EnumerationType,
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
  Integer,
  Tuple,
};

std::ostream& operator<<(std::ostream& os, const Kind& kind);
std::istream& operator>>(std::istream& is, std::optional<Kind>& value);

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

  Ref(MetaClass& meta, bool is_weak = false) : kind(meta.get_kind()), _is_weak(is_weak), ref(&meta) {
  }

  template <class OtherMeta, class = std::enable_if_t<std::is_base_of_v<MetaClass, OtherMeta>>>
  Ref(const Ref<OtherMeta>& other) : kind(other.get_kind()), _is_weak(other.is_weak()) {
    if (auto ptr = other.get(); ptr != nullptr) {
      ref = ptr;
    } else {
      ref = other.get_id();
    }
  }

  bool is_weak() const {
    return _is_weak;
  }

  Ref<MetaClass> as_weak_ref() const& {
    if (ref.index() == 0) {
      return {std::get<0>(ref), kind, true};
    } else {
      return {*std::get<1>(ref), true};
    }
  }

  Ref<MetaClass> as_weak_ref() && {
    _is_weak = true;
    return *this;
  }

  Kind get_kind() const {
    return kind;
  }

  template <class _MetaClass = MetaClass, class = std::enable_if_t<!std::is_const_v<_MetaClass>>>
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
      return std::get<1>(ref)->get_id();
    }
  }

  void set(MetaClass& meta) {
    kind = meta.get_kind();
    ref  = &meta;
  }

  MetaClass* get() const {
    if (ref.index() == 0) {
      return nullptr;
    } else {
      return std::get<1>(ref);
    }
  }

  inline MetaClass* operator->() const {
    return get();
  }

  inline MetaClass& operator*() const {
    return *get();
  }

  inline bool is_valid() const {
    return get_id() != meta_id_t::invalid && get_kind() != Kind::Unknown;
  }
};

template <class MetaClass>
inline bool operator==(const Ref<MetaClass>& lhs, const Ref<MetaClass>& rhs) {
  return lhs.get_kind() == rhs.get_kind() && lhs.get_id() == rhs.get_id();
}

class Meta {
  Kind kind;
  bool retained;
  meta_id_t id;

 protected:
  std::vector<Ref<Meta>> refs;

  inline Meta(meta_id_t id, Kind kind, std::vector<Ref<Meta>> refs)
      : kind(kind), retained(false), id(id), refs(std::move(refs)) {
  }

 public:
  static inline bool classof(Kind kind) {
    return true;
  }

  virtual ~Meta();

  inline bool isa(Kind expected_kind) const {
    return kind == expected_kind;
  }

  inline Kind get_kind() const {
    return kind;
  }

  inline bool is_retained() const {
    return retained;
  }

  inline void set_retained(bool new_retained) {
    retained = new_retained;
  }

  inline meta_id_t get_id() const {
    return id;
  }

  inline void set_id(meta_id_t new_id) {
    id = new_id;
  }

  inline const std::vector<Ref<Meta>>& get_refs() const {
    return refs;
  }

  inline std::vector<Ref<Meta>>& get_refs() {
    return refs;
  }

  bool contains_weak_refs() const;
};

std::unique_ptr<Meta> make_meta(Kind kind, std::vector<Ref<Meta>> refs);

template <typename Visitor>
bool visit_meta(Ref<const Meta> init, Visitor&& visitor) {
  auto worklist = std::queue<Ref<const Meta>>{};
  worklist.push(init);
  while (!worklist.empty()) {
    auto current = worklist.front();
    worklist.pop();
    if (visitor(current)) {
      if (!current.is_weak()) {
        for (const auto& ref : current->get_refs()) {
          worklist.emplace(ref);
        }
      }
    } else {
      return false;
    }
  }
  return true;
}

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
  return To::classof(meta->get_kind()) ? static_cast<const To*>(meta) : nullptr;
}

template <class To, class = std::enable_if_t<std::is_base_of_v<Meta, To>>>
Ref<To> dyn_cast(const Ref<Meta>& ref) {
  if (auto ptr = ref.get(); ptr != nullptr) {
    if (auto casted = dyn_cast<To>(ptr); casted != nullptr) {
      return {*casted, ref.is_weak()};
    }
  } else {
    if (To::classof(ref.get_kind())) {
      return {ref.get_id(), ref.get_kind(), ref.is_weak()};
    }
  }
  return {};
}

class Integer final : public Meta {
  META_CLASS(Meta, Integer, 0)
  int64_t data;

 public:
  inline Integer(meta_id_t id, int64_t data) : Meta(id, Kind::Integer, {}), data(data) {
  }

  inline int64_t get_data() const {
    return data;
  }

  inline void set_data(int64_t new_data) {
    data = new_data;
  }

  virtual ~Integer();
};

class String final : public Meta {
  META_CLASS(Meta, String, 0)
  std::string data;

 public:
  inline String(meta_id_t id, std::string data) : Meta(id, Kind::String, {}), data(std::move(data)) {
  }

  inline const std::string& get_data() const {
    return data;
  }

  inline void set_data(std::string new_data) {
    data = std::move(new_data);
  }

  virtual ~String();
};

inline bool operator==(const Meta& lhs, const Meta& rhs) {
  if (lhs.get_kind() != rhs.get_kind()) {
    return false;
  }
  if (lhs.get_id() == rhs.get_id()) {
    return true;
  }
  switch (lhs.get_kind()) {
    case Kind::Integer:
      return static_cast<const Integer&>(lhs).get_data() == static_cast<const Integer&>(rhs).get_data();
    case Kind::String:
      return static_cast<const String&>(lhs).get_data() == static_cast<const String&>(rhs).get_data();
    default:
      return lhs.get_refs() == rhs.get_refs();
  }
}

class Tuple final : public Meta {
  META_CLASS(Meta, Tuple, 0)

 public:
  inline Tuple(meta_id_t id, std::vector<Ref<Meta>> refs) : Meta(id, Kind::Tuple, std::move(refs)) {
  }

  inline auto size() const {
    return get_refs().size();
  }

  inline auto begin() {
    return get_refs().begin();
  }

  inline auto end() {
    return get_refs().end();
  }

  inline auto begin() const {
    return get_refs().begin();
  }

  inline auto end() const {
    return get_refs().end();
  }

  virtual ~Tuple();
};

class Node : public Meta {
 protected:
  inline Node(meta_id_t id, Kind kind, std::vector<Ref<Meta>> refs) : Meta(id, kind, std::move(refs)) {
  }

 public:
  static inline bool classof(Kind kind) {
    switch (kind) {
      case Kind::GlobalOrBuiltin:
      case Kind::CompileUnit:
      case Kind::File:
      case Kind::Subprogram:
      case Kind::Namespace:
      case Kind::VoidType:
      case Kind::BasicType:
      case Kind::StructureType:
      case Kind::UnionType:
      case Kind::ArrayType:
      case Kind::EnumerationType:
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
      case Kind::GlobalOrBuiltin:
      case Kind::CompileUnit:
      case Kind::File:
      case Kind::Subprogram:
      case Kind::Namespace:
      case Kind::VoidType:
      case Kind::BasicType:
      case Kind::StructureType:
      case Kind::UnionType:
      case Kind::ArrayType:
      case Kind::EnumerationType:
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
  META_CLASS(di::Node, Subrange, 2)
  META_INTEGER(0, size_t, lower_bound)
  META_INTEGER(1, size_t, count)

 public:
  virtual ~Subrange();
};

class Enumerator final : public di::Node {
  META_CLASS(di::Node, Enumerator, 3)
  META_STRING(0, name)
  META_INTEGER(1, ssize_t, value)
  META_INTEGER(2, bool, is_unsigned)

 public:
  virtual ~Enumerator();
};

class Scope : public di::Node {
 protected:
  inline Scope(meta_id_t id, Kind kind, std::vector<Ref<Meta>> refs) : di::Node(id, kind, std::move(refs)) {
  }

 public:
  static inline bool classof(Kind kind) {
    switch (kind) {
      case Kind::GlobalOrBuiltin:
      case Kind::CompileUnit:
      case Kind::File:
      case Kind::Subprogram:
      case Kind::Namespace:
      case Kind::VoidType:
      case Kind::BasicType:
      case Kind::StructureType:
      case Kind::UnionType:
      case Kind::ArrayType:
      case Kind::EnumerationType:
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

std::ostream& operator<<(std::ostream& os, const Language& language);
std::istream& operator>>(std::istream& is, std::optional<Language>& value);

class GlobalOrBuiltin final : public di::Scope {
  META_CLASS(di::Scope, GlobalOrBuiltin, 0)

 public:
  virtual ~GlobalOrBuiltin();
};

class File final : public di::Scope {
  META_CLASS(di::Scope, File, 2)
  META_STRING(0, filename)
  META_STRING(1, directory)

 public:
  bool is_unknown() const;

  virtual ~File();
};

class CompileUnit final : public di::Scope {
  META_CLASS(di::Scope, CompileUnit, 5)
  META_REF(0, di::File, file)
  META_STRING(1, producer)
  META_INTEGER(2, Language, language)
  META_INTEGER(3, bool, is_optimized)
  META_INTEGER(4, size_t, runtime_version)

 public:
  virtual ~CompileUnit();
};

class Namespace final : public di::Scope {
  META_CLASS(di::Scope, Namespace, 2)
  META_STRING(0, name)
  META_REF(1, di::Scope, scope)

 public:
  virtual ~Namespace();
};

class Type : public di::Scope {
 protected:
  inline Type(meta_id_t id, Kind kind, std::vector<Ref<Meta>> refs) : di::Scope(id, kind, std::move(refs)) {
  }

 public:
  static inline bool classof(Kind kind) {
    switch (kind) {
      case Kind::VoidType:
      case Kind::BasicType:
      case Kind::StructureType:
      case Kind::UnionType:
      case Kind::ArrayType:
      case Kind::EnumerationType:
      case Kind::DerivedType:
      case Kind::SubroutineType:
        return true;
      default:
        return false;
    }
  }

  inline bool is_void_type() const {
    return get_kind() == Kind::VoidType;
  }

  inline bool is_basic_type() const {
    return get_kind() == Kind::BasicType;
  }

  inline bool is_structure_type() const {
    return get_kind() == Kind::StructureType;
  }

  inline bool is_union_type() const {
    return get_kind() == Kind::UnionType;
  }

  inline bool is_array_type() const {
    return get_kind() == Kind::ArrayType;
  }

  inline bool is_derived_type() const {
    return get_kind() == Kind::DerivedType;
  }

  inline bool is_subroutine_type() const {
    return get_kind() == Kind::SubroutineType;
  }

  virtual std::string get_pretty_name() const = 0;

  virtual size_t get_size_in_bits() const = 0;

  // Strips away typedef, const, restrict and volatile
  const di::Type& get_canonical_type() const;

  virtual ~Type();
};

class VoidType final : public di::Type {
  META_CLASS(di::Type, VoidType, 0)

 public:
  std::string get_pretty_name() const override;
  size_t get_size_in_bits() const override;

  virtual ~VoidType();
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

std::ostream& operator<<(std::ostream& os, const Encoding& encoding);
std::istream& operator>>(std::istream& is, std::optional<Encoding>& value);

class BasicType final : public di::Type {
  META_CLASS(di::Type, BasicType, 3)
  META_STRING(0, name)
  META_INTEGER(1, Encoding, encoding)
  META_INTEGER_OVERRIDE(2, size_t, size_in_bits)

 public:
  std::string get_pretty_name() const override;

  virtual ~BasicType();
};

class DerivedType;
class StructureType final : public di::Type {
  META_CLASS(di::Type, StructureType, 7)
  META_STRING(0, name)
  META_STRING(1, identifier)
  META_REF(2, di::File, file)
  META_REF(3, di::Scope, scope)
  META_INTEGER(4, size_t, line)
  META_INTEGER_OVERRIDE(5, size_t, size_in_bits)
  META_TUPLE(6, di::Node, element)

 public:
  std::string get_pretty_name() const override;

  const di::DerivedType* find_member(size_t offset_in_bits) const;

  virtual ~StructureType();
};

class UnionType final : public di::Type {
  META_CLASS(di::Type, UnionType, 7)
  META_STRING(0, name)
  META_STRING(1, identifier)
  META_REF(2, di::File, file)
  META_REF(3, di::Scope, scope)
  META_INTEGER(4, size_t, line)
  META_INTEGER_OVERRIDE(5, size_t, size_in_bits)
  META_TUPLE(6, di::Node, element)

 public:
  std::string get_pretty_name() const override;

  virtual ~UnionType();
};

class ArrayType final : public di::Type {
  META_CLASS(di::Type, ArrayType, 3)
  META_REF(0, di::Type, base_type)
  META_INTEGER_OVERRIDE(1, size_t, size_in_bits)
  META_TUPLE(2, Integer, count)

 public:
  size_t get_flattened_count() const;
  std::string get_pretty_name() const override;

  virtual ~ArrayType();
};

class EnumerationType final : public di::Type {
  META_CLASS(di::Type, EnumerationType, 7)
  META_STRING(0, name)
  META_STRING(1, identifier)
  META_REF(2, di::File, file)
  META_REF(3, di::Scope, scope)
  META_INTEGER(4, size_t, line)
  META_INTEGER_OVERRIDE(5, size_t, size_in_bits)
  META_TUPLE(6, di::Node, element)

 public:
  std::string get_pretty_name() const override;

  virtual ~EnumerationType();
};

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

std::ostream& operator<<(std::ostream& os, const DerivedKind& kind);
std::istream& operator>>(std::istream& is, std::optional<DerivedKind>& value);

class DerivedType final : public di::Type {
  META_CLASS(di::Type, DerivedType, 8)
  META_STRING(0, name)
  META_REF(1, di::File, file)
  META_REF(2, di::Scope, scope)
  META_REF(3, di::Type, base_type)
  META_INTEGER(4, DerivedKind, tag)
  META_INTEGER(5, size_t, line)
  META_INTEGER(6, size_t, offset_in_bits)
  META_INTEGER_OVERRIDE(7, size_t, size_in_bits)

 public:
  std::string get_pretty_name() const override;

  virtual ~DerivedType();
};

class SubroutineType final : public di::Type {
  META_CLASS(di::Type, SubroutineType, 2)
  META_REF(0, di::Type, return_type)
  META_TUPLE(1, di::Type, argument_type)

 public:
  std::string get_pretty_name() const override;

  size_t get_size_in_bits() const override {
    return 0;
  }

  virtual ~SubroutineType();
};

template <class Visitor>
typename Visitor::return_type visit_type(const Type& type, Visitor&& visitor) {
  switch (type.get_kind()) {
    case Kind::VoidType:
      return std::forward<Visitor>(visitor)(static_cast<const VoidType&>(type));
    case Kind::BasicType:
      return std::forward<Visitor>(visitor)(static_cast<const BasicType&>(type));
    case Kind::StructureType:
      return std::forward<Visitor>(visitor)(static_cast<const StructureType&>(type));
    case Kind::UnionType:
      return std::forward<Visitor>(visitor)(static_cast<const UnionType&>(type));
    case Kind::ArrayType:
      return std::forward<Visitor>(visitor)(static_cast<const ArrayType&>(type));
    case Kind::EnumerationType:
      return std::forward<Visitor>(visitor)(static_cast<const EnumerationType&>(type));
    case Kind::DerivedType:
      return std::forward<Visitor>(visitor)(static_cast<const DerivedType&>(type));
    case Kind::SubroutineType:
      return std::forward<Visitor>(visitor)(static_cast<const SubroutineType&>(type));
    default:
      abort();
  }
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

  virtual const di::File& get_file() const = 0;

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
  META_CLASS(di::LexicalBlockBase, LexicalBlock, 4)
  META_REF(0, di::Scope, scope)
  META_REF(1, di::File, file)
  META_INTEGER(2, size_t, line)
  META_INTEGER(3, size_t, column)

 public:
  virtual ~LexicalBlock();
};

class LexicalBlockFile final : public di::LexicalBlockBase {
  META_CLASS(di::LexicalBlockBase, LexicalBlockFile, 3)
  META_REF(0, di::Scope, scope)
  META_REF(1, di::File, file)
  META_INTEGER(2, size_t, discriminator)

 public:
  virtual ~LexicalBlockFile();
};

class Subprogram final : public di::LocalScope {
  META_CLASS(di::LocalScope, Subprogram, 6)
  META_STRING(0, name)
  META_STRING(1, linkage_name)
  META_REF(2, di::File, file)
  META_REF(3, di::Scope, scope)
  META_REF(4, di::SubroutineType, type)
  META_INTEGER(5, size_t, line)

 public:
  virtual ~Subprogram();
};

class Location final : public meta::Node {
  META_CLASS(meta::Node, Location, 3)
  META_REF(0, di::LocalScope, scope)
  META_INTEGER(1, size_t, line)
  META_INTEGER(2, size_t, column)

 public:
  virtual ~Location();
};

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
  META_CLASS(Variable, LocalVariable, 6)
  META_STRING(0, name)
  META_STRING(1, linkage_name)
  META_REF(2, Scope, scope)
  META_REF(3, File, file)
  META_REF(4, Type, type)
  META_INTEGER(5, size_t, line)

 public:
  virtual ~LocalVariable();
};

class GlobalVariable final : public Variable {
  META_CLASS(Variable, GlobalVariable, 8)
  META_STRING(0, name)
  META_STRING(1, linkage_name)
  META_REF(2, Scope, scope)
  META_REF(3, File, file)
  META_REF(4, Type, type)
  META_INTEGER(5, size_t, line)
  META_INTEGER(6, bool, is_local)
  META_INTEGER(7, bool, is_definition)

 public:
  virtual ~GlobalVariable();
};

}  // namespace di

class Allocation : public Meta {
 public:
  inline Allocation(meta_id_t id, Kind kind, std::vector<Ref<Meta>> refs) : Meta(id, kind, std::move(refs)) {
  }

  virtual const di::Type& get_type() const = 0;
  virtual const di::File& get_file() const = 0;
  virtual size_t get_line() const          = 0;

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

  const di::Type& get_type() const override {
    return get_local_variable().get_type();
  }

  const di::File& get_file() const override {
    return get_local_variable().get_file();
  }

  size_t get_line() const override {
    return get_local_variable().get_line();
  }

 public:
  virtual ~StackAllocation();
};

class HeapAllocation final : public Allocation {
  META_CLASS(Allocation, HeapAllocation, 2)
  META_REF_OVERRIDE(0, di::Type, type)
  META_REF(1, di::Location, location)

  const di::File& get_file() const override {
    return get_location().get_scope().get_file();
  }

  size_t get_line() const override {
    return get_location().get_line();
  }

 public:
  virtual ~HeapAllocation();
};

class GlobalAllocation final : public Allocation {
  META_CLASS(Allocation, GlobalAllocation, 1)
  META_REF(0, di::GlobalVariable, global_variable)

  inline const di::Type& get_type() const override {
    return get_global_variable().get_type();
  }

  const di::File& get_file() const override {
    return get_global_variable().get_file();
  }

  size_t get_line() const override {
    return get_global_variable().get_line();
  }

 public:
  virtual ~GlobalAllocation();
};

}  // namespace meta

}  // namespace typeart
