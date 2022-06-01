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

#include "Macros.hpp"
#include "Types.h"

#include <cassert>
#include <iosfwd>
#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <variant>
#include <vector>

namespace llvm {
class Metadata;
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
  Inheritance,
  Member,
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
  std::variant<meta_id_t, MetaClass*> ref;

 public:
  Ref() : kind(Kind::Unknown), ref(meta_id_t::invalid) {
  }

  Ref(meta_id_t id, Kind kind) : kind(kind), ref(id) {
  }

  Ref(MetaClass& meta) : kind(meta.get_kind()), ref(&meta) {
  }

  template <class OtherMeta, class = std::enable_if_t<std::is_base_of_v<MetaClass, OtherMeta>>>
  Ref(const Ref<OtherMeta>& other) : kind(other.get_kind()) {
    if (auto ptr = other.get(); ptr != nullptr) {
      ref = ptr;
    } else {
      ref = other.get_id();
    }
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
  meta_id_t id;

 protected:
  std::vector<Ref<Meta>> refs;

  inline Meta(meta_id_t id, Kind kind, std::vector<Ref<Meta>> refs) : kind(kind), id(id), refs(std::move(refs)) {
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
};

bool operator==(const Meta& lhs, const Meta& rhs);

std::unique_ptr<Meta> make_meta(Kind kind, std::vector<Ref<Meta>> refs);

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
      return {*casted};
    }
  } else {
    if (To::classof(ref.get_kind())) {
      return {ref.get_id(), ref.get_kind()};
    }
  }
  return {};
}

class Integer final : public Meta {
  META_CLASS(Meta, Integer)
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
};

class String final : public Meta {
  META_CLASS(Meta, String)
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
};

class Tuple final : public Meta {
  META_CLASS(Meta, Tuple)

 public:
  using size_type = std::vector<Ref<Meta>>::size_type;

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
};

template <class MetaClass>
class TupleProxy final {
  const Tuple* tuple;

 public:
  using size_type = Tuple::size_type;

 public:
  class const_iterator {
    using base_type = std::vector<Ref<Meta>>::const_iterator;
    base_type it;

   public:
    using value_type        = MetaClass;
    using reference         = MetaClass&;
    using pointer           = MetaClass*;
    using difference_type   = base_type::difference_type;
    using iterator_category = std::random_access_iterator_tag;

   public:
    const_iterator(base_type it) : it(std::move(it)) {
    }

    const_iterator& operator++() {
      ++it;
      return *this;
    }

    const_iterator operator++(int) {
      return {it++};
    }

    const_iterator& operator--() {
      --it;
      return *this;
    }

    const_iterator operator--(int) {
      return {it--};
    }

    const_iterator& operator+=(size_type offset) {
      it += offset;
      return *this;
    }

    friend const_iterator operator+(const const_iterator& other, size_type offset) {
      return {other.it + offset};
    }

    friend const_iterator operator+(size_type offset, const const_iterator& other) {
      return {offset + other.it};
    }

    const_iterator& operator-=(size_type offset) {
      it -= offset;
      return *this;
    }

    friend const_iterator operator-(const const_iterator& other, size_type offset) {
      return {other - offset};
    }

    friend difference_type operator-(const const_iterator& lhs, const const_iterator& rhs) {
      return lhs.it - rhs.it;
    }

    reference operator*() const {
      return *dyn_cast<MetaClass>(it->get());
    }

    pointer operator->() const {
      return dyn_cast<MetaClass>(it->get());
    }

    reference operator[](size_type n) const {
      return *dyn_cast<MetaClass>(it[n].get());
    }

    friend bool operator==(const const_iterator& lhs, const const_iterator& rhs) {
      return lhs.it == rhs.it;
    }

    friend bool operator!=(const const_iterator& lhs, const const_iterator& rhs) {
      return !(lhs == rhs);
    }

    friend bool operator<(const const_iterator& lhs, const const_iterator& rhs) {
      return lhs.it < rhs.it;
    }

    friend bool operator>(const const_iterator& lhs, const const_iterator& rhs) {
      return lhs.it > rhs.it;
    }

    friend bool operator<=(const const_iterator& lhs, const const_iterator& rhs) {
      return lhs < rhs || lhs == rhs;
    }

    friend bool operator>=(const const_iterator& lhs, const const_iterator& rhs) {
      return lhs == rhs || lhs > rhs;
    }
  };

 public:
  TupleProxy(const Tuple& tuple) : tuple(&tuple) {
  }

  inline auto size() const {
    return tuple->size();
  }

  inline const_iterator begin() const {
    return {tuple->begin()};
  }

  inline const_iterator end() const {
    return {tuple->end()};
  }
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
      case Kind::Inheritance:
      case Kind::Member:
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
      case Kind::Inheritance:
      case Kind::Member:
        return true;
      default:
        return false;
    }
  }

  virtual ~Node();
};

class File;
class Scope;
class Type;

class Subrange final : public di::Node {
  META_CLASS(di::Node, Subrange,
             ((INTEGER, size_t, lower_bound),  //
              (INTEGER, size_t, count)))
};

class Enumerator final : public di::Node {
  META_CLASS(di::Node, Enumerator,
             ((STRING, name),             //
              (INTEGER, ssize_t, value),  //
              (INTEGER, bool, is_unsigned)))
};

class StructureType;

class Inheritance final : public di::Node {
  META_CLASS(di::Node, Inheritance,
             ((REF, di::Scope, scope),  //
              (REF, di::Type, base),    //
              (INTEGER, size_t, offset_in_bits)))

 public:
  const StructureType& get_base_structure_type() const;
};

class Member final : public di::Node {
  META_CLASS(di::Node, Member,
             ((STRING, name),                     //
              (REF, di::File, file),              //
              (REF, di::Scope, scope),            //
              (REF, di::Type, type),              //
              (INTEGER, size_t, line),            //
              (INTEGER, size_t, offset_in_bits),  //
              (INTEGER, size_t, size_in_bits)))

  std::string get_pretty_name() const;
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

  virtual std::string get_pretty_name() const = 0;

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
  META_CLASS(di::Scope, GlobalOrBuiltin)

 public:
  std::string get_pretty_name() const override;
};

class File final : public di::Scope {
  META_CLASS(di::Scope, File,
             ((STRING, filename),  //
              (STRING, directory)))

 public:
  bool is_unknown() const;

  std::string get_pretty_name() const override;
};

class CompileUnit final : public di::Scope {
  META_CLASS(di::Scope, CompileUnit,
             ((REF, di::File, file),          //
              (STRING, producer),             //
              (INTEGER, Language, language),  //
              (INTEGER, bool, is_optimized),  //
              (INTEGER, size_t, runtime_version)))

 public:
  std::string get_pretty_name() const override;
};

class Namespace final : public di::Scope {
  META_CLASS(di::Scope, Namespace,
             ((STRING, name),  //
              (REF, di::Scope, scope)))

 public:
  std::string get_pretty_name() const override;
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

  inline bool is_enumeration_type() const {
    return get_kind() == Kind::EnumerationType;
  }

  inline bool is_derived_type() const {
    return get_kind() == Kind::DerivedType;
  }

  inline bool is_subroutine_type() const {
    return get_kind() == Kind::SubroutineType;
  }

  virtual size_t get_size_in_bits() const = 0;

  const di::Type& strip_typedefs_and_qualifiers() const;

  virtual ~Type();
};

class VoidType final : public di::Type {
  META_CLASS(di::Type, VoidType)

 public:
  std::string get_pretty_name() const override;
  size_t get_size_in_bits() const override;
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
  META_CLASS(di::Type, BasicType,
             ((STRING, name),                 //
              (INTEGER, Encoding, encoding),  //
              (INTEGER, size_t, size_in_bits, override)))

 public:
  std::string get_pretty_name() const override;
};

class Subprogram;

class StructureType final : public di::Type {
  META_CLASS(di::Type, StructureType,
             ((STRING, name),                             //
              (STRING, identifier),                       //
              (REF, di::File, file),                      //
              (REF, di::Scope, scope),                    //
              (INTEGER, size_t, line),                    //
              (INTEGER, size_t, size_in_bits, override),  //
              (TUPLE, di::Inheritance, base_classes),     //
              (TUPLE, di::Subprogram, methods),           //
              (TUPLE, di::Member, direct_members)))

 public:
  std::string get_pretty_name() const override;

  const di::Inheritance* find_inheritance(size_t offset_in_bits) const;

  const di::Member* find_member(size_t offset_in_bits) const;
};

class UnionType final : public di::Type {
  META_CLASS(di::Type, UnionType,
             ((STRING, name),                             //
              (STRING, identifier),                       //
              (REF, di::File, file),                      //
              (REF, di::Scope, scope),                    //
              (INTEGER, size_t, line),                    //
              (INTEGER, size_t, size_in_bits, override),  //
              (TUPLE, di::Subprogram, methods),           //
              (TUPLE, di::Member, members)))

 public:
  std::string get_pretty_name() const override;
};

class ArrayType final : public di::Type {
  META_CLASS(di::Type, ArrayType,
             ((REF, di::Type, base_type),                 //
              (INTEGER, size_t, size_in_bits, override),  //
              (TUPLE, Integer, counts)))

 public:
  size_t get_flattened_count() const;
  std::string get_pretty_name() const override;
};

class EnumerationType final : public di::Type {
  META_CLASS(di::Type, EnumerationType,
             ((STRING, name),                             //
              (STRING, identifier),                       //
              (REF, di::File, file),                      //
              (REF, di::Scope, scope),                    //
              (INTEGER, size_t, line),                    //
              (INTEGER, size_t, size_in_bits, override),  //
              (TUPLE, di::Node, elements)))

 public:
  std::string get_pretty_name() const override;
};

enum class DerivedKind {
  Typedef,
  Pointer,
  Reference,
  RvalueReference,
  Const,
  Restrict,
  Volatile,
  PtrToMemberType,
};

std::ostream& operator<<(std::ostream& os, const DerivedKind& kind);
std::istream& operator>>(std::istream& is, std::optional<DerivedKind>& value);

class DerivedType final : public di::Type {
  META_CLASS(di::Type, DerivedType,
             ((STRING, name),                     //
              (REF, di::File, file),              //
              (REF, di::Scope, scope),            //
              (REF, di::Type, base_type),         //
              (INTEGER, DerivedKind, tag),        //
              (INTEGER, size_t, line),            //
              (INTEGER, size_t, offset_in_bits),  //
              (INTEGER, size_t, size_in_bits, override)))

 public:
  std::string get_pretty_name() const override;
};

class SubroutineType final : public di::Type {
  META_CLASS(di::Type, SubroutineType,
             ((REF, di::Type, return_type),  //
              (TUPLE, di::Type, argument_types)))

 public:
  std::string get_pretty_name() const override;

  size_t get_size_in_bits() const override {
    return 0;
  }
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
  META_CLASS(di::LexicalBlockBase, LexicalBlock,
             ((REF, di::Scope, scope),          //
              (REF, di::File, file, override),  //
              (INTEGER, size_t, line),          //
              (INTEGER, size_t, column)))

 public:
  std::string get_pretty_name() const override;
};

class LexicalBlockFile final : public di::LexicalBlockBase {
  META_CLASS(di::LexicalBlockBase, LexicalBlockFile,
             ((REF, di::Scope, scope),          //
              (REF, di::File, file, override),  //
              (INTEGER, size_t, discriminator)))

 public:
  std::string get_pretty_name() const override;
};

class Subprogram final : public di::LocalScope {
  META_CLASS(di::LocalScope, Subprogram,
             ((STRING, name),                   //
              (STRING, linkage_name),           //
              (REF, di::File, file, override),  //
              (REF, di::Scope, scope),          //
              (REF, di::SubroutineType, type),  //
              (INTEGER, size_t, line)))

 public:
  std::string get_pretty_name() const override;
};

class Location final : public meta::Node {
  META_CLASS(meta::Node, Location,
             ((REF, di::LocalScope, scope),  //
              (INTEGER, size_t, line),       //
              (INTEGER, size_t, column)))
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
  META_CLASS(Variable, LocalVariable,
             ((STRING, name),          //
              (STRING, linkage_name),  //
              (REF, Scope, scope),     //
              (REF, File, file),       //
              (REF, Type, type),       //
              (INTEGER, size_t, line)))
};

class GlobalVariable final : public Variable {
  META_CLASS(Variable, GlobalVariable,
             ((STRING, name),             //
              (STRING, linkage_name),     //
              (REF, Scope, scope),        //
              (REF, File, file),          //
              (REF, Type, type),          //
              (INTEGER, size_t, line),    //
              (INTEGER, bool, is_local),  //
              (INTEGER, bool, is_definition)))
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
  META_CLASS(Allocation, StackAllocation,
             ((REF, di::LocalVariable, local_variable),  //
              (REF, di::Location, location)))

  const di::Type& get_type() const override {
    return get_local_variable().get_type();
  }

  const di::File& get_file() const override {
    return get_local_variable().get_file();
  }

  size_t get_line() const override {
    return get_local_variable().get_line();
  }
};

class HeapAllocation final : public Allocation {
  META_CLASS(Allocation, HeapAllocation,
             ((REF, di::Type, type, override),  //
              (REF, di::Location, location)))

  const di::File& get_file() const override {
    return get_location().get_scope().get_file();
  }

  size_t get_line() const override {
    return get_location().get_line();
  }
};

class GlobalAllocation final : public Allocation {
  META_CLASS(Allocation, GlobalAllocation, ((REF, di::GlobalVariable, global_variable)))

  inline const di::Type& get_type() const override {
    return get_global_variable().get_type();
  }

  const di::File& get_file() const override {
    return get_global_variable().get_file();
  }

  size_t get_line() const override {
    return get_global_variable().get_line();
  }
};

}  // namespace meta

}  // namespace typeart
