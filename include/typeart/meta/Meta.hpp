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
#include "Meta.h"

#include <cassert>
#include <iosfwd>
#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <variant>
#include <vector>

namespace meta {

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

  explicit byte_size(value_type value) : _value(value) {
  }

 public:
  static byte_size from_bits(value_type bits) {
    assert(bits % 8 == 0);
    return byte_size{bits / 8};
  }

  static byte_size from_bytes(value_type bytes) {
    return byte_size{bytes};
  }

  value_type value() const {
    return _value;
  }

  value_type as_bits() const {
    return _value * 8;
  }
};

std::ostream& operator<<(std::ostream& os, const byte_size& value);

inline byte_size operator*(size_t lhs, byte_size rhs) {
  return byte_size::from_bytes(lhs * rhs.value());
}

inline pointer operator+(pointer p, byte_size offset) {
  return pointer{static_cast<const int8_t*>(p.get()) + offset.value()};
}

class byte_offset {
  using value_type = ssize_t;

  value_type _value;

  explicit byte_offset(value_type value) : _value(value) {
  }

 public:
  static byte_offset zero;

  static byte_offset from_bits(value_type bits) {
    assert(bits % 8 == 0);
    return byte_offset{bits / 8};
  }

  static byte_offset from_bytes(value_type bytes) {
    return byte_offset{bytes};
  }

  value_type value() const {
    return _value;
  }

  value_type as_bits() const {
    return _value * 8;
  }
};

std::ostream& operator<<(std::ostream& os, const byte_offset& value);

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
  Tuple,
  Optional,
};

static const Kind KINDS[] = {
    Kind::Unknown,        Kind::GlobalOrBuiltin, Kind::CompileUnit,      Kind::File,
    Kind::Subprogram,     Kind::Namespace,       Kind::VoidType,         Kind::BasicType,
    Kind::StructureType,  Kind::UnionType,       Kind::ArrayType,        Kind::EnumerationType,
    Kind::DerivedType,    Kind::SubroutineType,  Kind::Location,         Kind::LocalVariable,
    Kind::GlobalVariable, Kind::HeapAllocation,  Kind::StackAllocation,  Kind::GlobalAllocation,
    Kind::Subrange,       Kind::LexicalBlock,    Kind::LexicalBlockFile, Kind::Enumerator,
    Kind::Inheritance,    Kind::Member,          Kind::String,           Kind::Tuple,
    Kind::Optional,
};

std::ostream& operator<<(std::ostream& os, const Kind& kind);
std::istream& operator>>(std::istream& is, std::optional<Kind>& value);

class Meta {
  Kind kind;
  meta_id_t id;

 protected:
  std::vector<Meta*> refs;

  inline Meta(meta_id_t id, Kind kind, std::vector<Meta*> refs) : kind(kind), id(id), refs(std::move(refs)) {
  }

  virtual bool compare(const Meta& other) const = 0;

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

  inline const std::vector<Meta*>& get_refs() const {
    return refs;
  }

  inline std::vector<Meta*>& get_refs() {
    return refs;
  }

  virtual const char* get_ref_name(size_t idx) const = 0;

  virtual size_t get_member_count() const                               = 0;
  virtual const char* get_member_name(size_t idx) const                 = 0;
  virtual std::string serialize_member(size_t idx) const                = 0;
  virtual void deserialize_member(size_t idx, const std::string& value) = 0;

  friend bool operator==(const Meta& lhs, const Meta& rhs);
};

std::unique_ptr<Meta> make_meta(Kind kind);

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

class String final : public Meta {
  META_CLASS(Meta, String,  //
             (0, ()),       //
             (0, ()))
  std::string data;

 public:
  inline String(meta_id_t id, std::string data) : Meta(id, Kind::String, {}), data(std::move(data)) {
  }

  inline operator const std::string&() const {
    return data;
  }

  inline const std::string& get_data() const {
    return data;
  }

  inline void set_data(std::string new_data) {
    data = std::move(new_data);
  }
};

class TupleBase : public Meta {
 protected:
  inline TupleBase(meta_id_t id, Kind kind, std::vector<Meta*> refs) : Meta(id, kind, std::move(refs)) {
  }

 public:
  static inline bool classof(Kind kind) {
    switch (kind) {
      case Kind::Tuple:
      case Kind::Optional:
        return true;
      default:
        return false;
    }
  }

  virtual ~TupleBase();

 public:
  using size_type = std::vector<Meta*>::size_type;

 public:
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

class Tuple final : public TupleBase {
  META_CLASS(TupleBase, Tuple,  //
             (0, ()),           //
             (0, ()))

 public:
  using size_type = std::vector<Meta*>::size_type;
};

template <class MetaClass>
class TupleProxy final {
  const Tuple* tuple;

 public:
  using size_type = Tuple::size_type;

 public:
  class const_iterator {
    using base_type = std::vector<Meta*>::const_iterator;
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
      return *dyn_cast<MetaClass>(*it);
    }

    pointer operator->() const {
      return dyn_cast<MetaClass>(*it);
    }

    reference operator[](size_type n) const {
      return *dyn_cast<MetaClass>(it[n]);
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

class Optional final : public TupleBase {
  META_CLASS(TupleBase, Optional,  //
             (0, ()),              //
             (0, ()))

 public:
  using size_type = std::vector<Meta*>::size_type;

  inline bool has_value() const {
    return get_refs().size() == 1;
  }

  inline Meta& value() const {
    return *get_refs()[0];
  }
};

template <class MetaClass>
class OptionalProxy final {
  const Optional* optional;

 public:
  using size_type = Optional::size_type;

 public:
  OptionalProxy(const Optional& optional) : optional(&optional) {
  }

  inline bool has_value() const {
    return optional->has_value();
  }

  inline MetaClass& value() const {
    return *dyn_cast<MetaClass>(&optional->value());
  }
};

class Node : public Meta {
 protected:
  inline Node(meta_id_t id, Kind kind, std::vector<Meta*> refs) : Meta(id, kind, std::move(refs)) {
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
  inline Node(meta_id_t id, Kind kind, std::vector<Meta*> refs) : meta::Node(id, kind, std::move(refs)) {
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
  META_CLASS(di::Node, Subrange,          //
             (0, ()),                     //
             (2, ((size_t, lower_bound),  //
                  (size_t, count))))
};

class Enumerator final : public di::Node {
  META_CLASS(di::Node, Enumerator,   //
             (1, ((STRING, name))),  //
             (2, ((ssize_t, value),  //
                  (bool, is_unsigned))))
};

class StructureType;

class Inheritance final : public di::Node {
  META_CLASS(di::Node, Inheritance,         //
             (2, ((REF, di::Scope, scope),  //
                  (REF, di::Type, base))),  //
             (1, ((size_t, offset_in_bits))))

 public:
  const StructureType& get_base_structure_type() const;
};

class Member final : public di::Node {
  META_CLASS(di::Node, Member,               //
             (4, ((STRING, name),            //
                  (REF, di::File, file),     //
                  (REF, di::Scope, scope),   //
                  (REF, di::Type, type))),   //
             (3, ((size_t, line),            //
                  (size_t, offset_in_bits),  //
                  (size_t, size_in_bits))))

  std::string get_pretty_name() const;
};

class Scope : public di::Node {
 protected:
  inline Scope(meta_id_t id, Kind kind, std::vector<Meta*> refs) : di::Node(id, kind, std::move(refs)) {
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
  META_CLASS(di::Scope, GlobalOrBuiltin,  //
             (0, ()),                     //
             (0, ()))

 public:
  std::string get_pretty_name() const override;
};

class File final : public di::Scope {
  META_CLASS(di::Scope, File,
             (2, ((STRING, filename),     //
                  (STRING, directory))),  //
             (0, ()))

 public:
  bool is_unknown() const;

  std::string get_pretty_name() const override;
};

class CompileUnit final : public di::Scope {
  META_CLASS(di::Scope, CompileUnit,
             (2, ((REF, di::File, file),  //
                  (STRING, producer))),   //
             (3, ((Language, language),   //
                  (bool, is_optimized),   //
                  (size_t, runtime_version))))

 public:
  std::string get_pretty_name() const override;
};

class Namespace final : public di::Scope {
  META_CLASS(di::Scope, Namespace,
             (2, ((STRING, name),             //
                  (REF, di::Scope, scope))),  //
             (0, ()))

 public:
  std::string get_pretty_name() const override;
};

class Type : public di::Scope {
 protected:
  inline Type(meta_id_t id, Kind kind, std::vector<Meta*> refs) : di::Scope(id, kind, std::move(refs)) {
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
  META_CLASS(di::Type, VoidType,  //
             (0, ()),             //
             (0, ()))

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
  META_CLASS(di::Type, BasicType,        //
             (1, ((STRING, name))),      //
             (2, ((Encoding, encoding),  //
                  (size_t, size_in_bits, override))))

 public:
  std::string get_pretty_name() const override;
};

class Subprogram;

class StructureType final : public di::Type {
  META_CLASS(di::Type, StructureType,
             (7, ((STRING, name),                          //
                  (STRING, identifier),                    //
                  (REF, di::File, file),                   //
                  (REF, di::Scope, scope),                 //
                  (TUPLE, di::Inheritance, base_classes),  //
                  (TUPLE, di::Subprogram, methods),        //
                  (TUPLE, di::Member, direct_members))),
             (2, ((size_t, line),                      //
                  (size_t, size_in_bits, override))))  //

 public:
  std::string get_pretty_name() const override;

  const di::Inheritance* find_inheritance(size_t offset_in_bits) const;

  const di::Member* find_member(size_t offset_in_bits) const;
};

class UnionType final : public di::Type {
  META_CLASS(di::Type, UnionType,
             (6, ((STRING, name),                    //
                  (STRING, identifier),              //
                  (REF, di::File, file),             //
                  (REF, di::Scope, scope),           //
                  (TUPLE, di::Subprogram, methods),  //
                  (TUPLE, di::Member, members))),
             (2, ((size_t, line),                      //
                  (size_t, size_in_bits, override))))  //

 public:
  std::string get_pretty_name() const override;
};

class ArrayType final : public di::Type {
  META_CLASS(di::Type, ArrayType,  //
             (1, ((REF, di::Type, base_type))),
             (2, ((size_t, size_in_bits, override),  //
                  (std::vector<size_t>, counts))))   //

 public:
  size_t get_flattened_count() const;
  std::string get_pretty_name() const override;
};

class EnumerationType final : public di::Type {
  META_CLASS(di::Type, EnumerationType,
             (5, ((STRING, name),           //
                  (STRING, identifier),     //
                  (REF, di::File, file),    //
                  (REF, di::Scope, scope),  //
                  (TUPLE, di::Node, elements))),
             (2, ((size_t, line),                      //
                  (size_t, size_in_bits, override))))  //

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
             (4, ((STRING, name),                //
                  (REF, di::File, file),         //
                  (REF, di::Scope, scope),       //
                  (REF, di::Type, base_type))),  //
             (4, ((DerivedKind, tag),            //
                  (size_t, line),                //
                  (size_t, offset_in_bits),      //
                  (size_t, size_in_bits, override))))

 public:
  std::string get_pretty_name() const override;
};

class SubroutineType final : public di::Type {
  META_CLASS(di::Type, SubroutineType,
             (2, ((REF, di::Type, return_type),         //
                  (TUPLE, di::Type, argument_types))),  //
             (0, ()))

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
  inline LocalScope(meta_id_t id, Kind kind, std::vector<Meta*> refs) : di::Scope(id, kind, std::move(refs)) {
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
  inline LexicalBlockBase(meta_id_t id, Kind kind, std::vector<Meta*> refs)
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
             (2, ((REF, di::Scope, scope),            //
                  (REF, di::File, file, override))),  //
             (2, ((size_t, line),                     //
                  (size_t, column))))

 public:
  std::string get_pretty_name() const override;
};

class LexicalBlockFile final : public di::LexicalBlockBase {
  META_CLASS(di::LexicalBlockBase, LexicalBlockFile,
             (2, ((REF, di::Scope, scope),            //
                  (REF, di::File, file, override))),  //
             (1, ((size_t, discriminator))))

 public:
  std::string get_pretty_name() const override;
};

class Subprogram final : public di::LocalScope {
  META_CLASS(di::LocalScope, Subprogram,
             (5, ((STRING, name),                     //
                  (STRING, linkage_name),             //
                  (REF, di::File, file, override),    //
                  (REF, di::Scope, scope),            //
                  (REF, di::SubroutineType, type))),  //
             (1, ((size_t, line))))

 public:
  std::string get_pretty_name() const override;
};

class Location final : public meta::Node {
  META_CLASS(meta::Node, Location,                 //
             (1, ((REF, di::LocalScope, scope))),  //
             (2, ((size_t, line),                  //
                  (size_t, column))))
};

class Variable : public di::Node {
 protected:
  inline Variable(meta_id_t id, Kind kind, std::vector<Meta*> refs) : di::Node(id, kind, std::move(refs)) {
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
             (5, ((STRING, name),          //
                  (STRING, linkage_name),  //
                  (REF, Scope, scope),     //
                  (REF, File, file),       //
                  (REF, Type, type))),     //
             (1, ((size_t, line))))
};

class GlobalVariable final : public Variable {
  META_CLASS(Variable, GlobalVariable,
             (5, ((STRING, name),          //
                  (STRING, linkage_name),  //
                  (REF, Scope, scope),     //
                  (REF, File, file),       //
                  (REF, Type, type))),     //
             (3, ((size_t, line),          //
                  (bool, is_local),        //
                  (bool, is_definition))))
};

}  // namespace di

class Allocation : public Meta {
 public:
  inline Allocation(meta_id_t id, Kind kind, std::vector<Meta*> refs) : Meta(id, kind, std::move(refs)) {
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
             (2, ((REF, di::LocalVariable, local_variable),  //
                  (REF, di::Location, location))),           //
             (1, ((std::optional<size_t>, count))))

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
             (2, ((REF, di::Type, type, override),  //
                  (REF, di::Location, location))),  //
             (0, ()))

  const di::File& get_file() const override {
    return get_location().get_scope().get_file();
  }

  size_t get_line() const override {
    return get_location().get_line();
  }
};

class GlobalAllocation final : public Allocation {
  META_CLASS(Allocation, GlobalAllocation,                       //
             (1, ((REF, di::GlobalVariable, global_variable))),  //
             (0, ()))

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
