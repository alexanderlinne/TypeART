#include "meta/Meta.hpp"

#include "meta/Database.hpp"
#include "support/Logger.hpp"

#include <iostream>
#include <limits>
#include <queue>

namespace meta {

const meta_id_t meta_id_t::invalid = {};

std::ostream& operator<<(std::ostream& os, const byte_size& value) {
  os << value.value() << "B";
  return os;
}

byte_offset byte_offset::zero = byte_offset{0};

std::ostream& operator<<(std::ostream& os, const byte_offset& value) {
  os << value.value() << "B";
  return os;
}

std::ostream& operator<<(std::ostream& os, const Kind& kind) {
  switch (kind) {
    case Kind::Unknown:
      os << "Unknown";
      break;
    case Kind::GlobalOrBuiltin:
      os << "GlobalOrBuiltin";
      break;
    case Kind::CompileUnit:
      os << "CompileUnit";
      break;
    case Kind::File:
      os << "File";
      break;
    case Kind::Subprogram:
      os << "Subprogram";
      break;
    case Kind::Namespace:
      os << "Namespace";
      break;
    case Kind::VoidType:
      os << "VoidType";
      break;
    case Kind::BasicType:
      os << "BasicType";
      break;
    case Kind::StructureType:
      os << "StructureType";
      break;
    case Kind::UnionType:
      os << "UnionType";
      break;
    case Kind::ArrayType:
      os << "ArrayType";
      break;
    case Kind::EnumerationType:
      os << "EnumerationType";
      break;
    case Kind::DerivedType:
      os << "DerivedType";
      break;
    case Kind::SubroutineType:
      os << "SubroutineType";
      break;
    case Kind::Location:
      os << "Location";
      break;
    case Kind::LocalVariable:
      os << "LocalVariable";
      break;
    case Kind::GlobalVariable:
      os << "GlobalVariable";
      break;
    case Kind::HeapAllocation:
      os << "HeapAllocation";
      break;
    case Kind::StackAllocation:
      os << "StackAllocation";
      break;
    case Kind::GlobalAllocation:
      os << "GlobalAllocation";
      break;
    case Kind::Subrange:
      os << "Subrange";
      break;
    case Kind::LexicalBlock:
      os << "LexicalBlock";
      break;
    case Kind::LexicalBlockFile:
      os << "LexicalBlockFile";
      break;
    case Kind::Enumerator:
      os << "Enumerator";
      break;
    case Kind::Inheritance:
      os << "Inheritance";
      break;
    case Kind::Member:
      os << "Member";
      break;
    case Kind::String:
      os << "String";
      break;
    case Kind::Integer:
      os << "Integer";
      break;
    case Kind::Tuple:
      os << "Tuple";
      break;
    case Kind::Optional:
      os << "Optional";
      break;
    default:
      LOG_FATAL("Missing switch case {}", static_cast<int>(kind));
      abort();
  }
  return os;
}

std::istream& operator>>(std::istream& is, std::optional<Kind>& value) {
  std::string input;
  is >> input;
  if (input == "Unknown") {
    value = Kind::Unknown;
  } else if (input == "GlobalOrBuiltin") {
    value = Kind::GlobalOrBuiltin;
  } else if (input == "CompileUnit") {
    value = Kind::CompileUnit;
  } else if (input == "File") {
    value = Kind::File;
  } else if (input == "Subprogram") {
    value = Kind::Subprogram;
  } else if (input == "Namespace") {
    value = Kind::Namespace;
  } else if (input == "VoidType") {
    value = Kind::VoidType;
  } else if (input == "BasicType") {
    value = Kind::BasicType;
  } else if (input == "StructureType") {
    value = Kind::StructureType;
  } else if (input == "UnionType") {
    value = Kind::UnionType;
  } else if (input == "EnumerationType") {
    value = Kind::EnumerationType;
  } else if (input == "ArrayType") {
    value = Kind::ArrayType;
  } else if (input == "DerivedType") {
    value = Kind::DerivedType;
  } else if (input == "SubroutineType") {
    value = Kind::SubroutineType;
  } else if (input == "Location") {
    value = Kind::Location;
  } else if (input == "LocalVariable") {
    value = Kind::LocalVariable;
  } else if (input == "GlobalVariable") {
    value = Kind::GlobalVariable;
  } else if (input == "HeapAllocation") {
    value = Kind::HeapAllocation;
  } else if (input == "StackAllocation") {
    value = Kind::StackAllocation;
  } else if (input == "GlobalAllocation") {
    value = Kind::GlobalAllocation;
  } else if (input == "Subrange") {
    value = Kind::Subrange;
  } else if (input == "LexicalBlock") {
    value = Kind::LexicalBlock;
  } else if (input == "LexicalBlockFile") {
    value = Kind::LexicalBlockFile;
  } else if (input == "Enumerator") {
    value = Kind::Enumerator;
  } else if (input == "Inheritance") {
    value = Kind::Inheritance;
  } else if (input == "Member") {
    value = Kind::Member;
  } else if (input == "String") {
    value = Kind::String;
  } else if (input == "Integer") {
    value = Kind::Integer;
  } else if (input == "Tuple") {
    value = Kind::Tuple;
  } else if (input == "Optional") {
    value = Kind::Optional;
  } else {
    value = {};
  }
  return is;
}

Meta::~Meta() {
}

bool operator==(const Meta& lhs, const Meta& rhs) {
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

std::unique_ptr<Meta> make_meta(Kind kind) {
  auto result = std::unique_ptr<Meta>{};
  switch (kind) {
    case Kind::String:
      result = std::make_unique<String>();
      break;
    case Kind::Integer:
      result = std::make_unique<Integer>();
      break;
    case Kind::Tuple:
      result = std::make_unique<Tuple>();
      break;
    case Kind::Optional:
      result = std::make_unique<Optional>();
      break;
    case Kind::GlobalOrBuiltin:
      result = std::make_unique<di::GlobalOrBuiltin>();
      break;
    case Kind::CompileUnit:
      result = std::make_unique<di::CompileUnit>();
      break;
    case Kind::File:
      result = std::make_unique<di::File>();
      break;
    case Kind::Subprogram:
      result = std::make_unique<di::Subprogram>();
      break;
    case Kind::Namespace:
      result = std::make_unique<di::Namespace>();
      break;
    case Kind::VoidType:
      result = std::make_unique<di::VoidType>();
      break;
    case Kind::BasicType:
      result = std::make_unique<di::BasicType>();
      break;
    case Kind::StructureType:
      result = std::make_unique<di::StructureType>();
      break;
    case Kind::UnionType:
      result = std::make_unique<di::UnionType>();
      break;
    case Kind::ArrayType:
      result = std::make_unique<di::ArrayType>();
      break;
    case Kind::EnumerationType:
      result = std::make_unique<di::EnumerationType>();
      break;
    case Kind::DerivedType:
      result = std::make_unique<di::DerivedType>();
      break;
    case Kind::SubroutineType:
      result = std::make_unique<di::SubroutineType>();
      break;
    case Kind::Location:
      result = std::make_unique<di::Location>();
      break;
    case Kind::LocalVariable:
      result = std::make_unique<di::LocalVariable>();
      break;
    case Kind::GlobalVariable:
      result = std::make_unique<di::GlobalVariable>();
      break;
    case Kind::HeapAllocation:
      result = std::make_unique<HeapAllocation>();
      break;
    case Kind::StackAllocation:
      result = std::make_unique<StackAllocation>();
      break;
    case Kind::GlobalAllocation:
      result = std::make_unique<GlobalAllocation>();
      break;
    case Kind::Subrange:
      result = std::make_unique<di::Subrange>();
      break;
    case Kind::LexicalBlock:
      result = std::make_unique<di::LexicalBlock>();
      break;
    case Kind::LexicalBlockFile:
      result = std::make_unique<di::LexicalBlockFile>();
      break;
    case Kind::Enumerator:
      result = std::make_unique<di::Enumerator>();
      break;
    case Kind::Inheritance:
      result = std::make_unique<di::Inheritance>();
      break;
    case Kind::Member:
      result = std::make_unique<di::Member>();
      break;
    default:
      LOG_FATAL("Missing switch case {}", kind);
      abort();
  }
  return result;
}

META_CLASS_IMPL(Meta, Integer)

META_CLASS_IMPL(Meta, String)

TupleBase::~TupleBase() {
}

META_CLASS_IMPL(Meta, Tuple)

META_CLASS_IMPL(TupleBase, Optional)

Node::~Node() {
}

namespace di {

Node::~Node() {
}

META_CLASS_IMPL(di::Node, Subrange,
                ((INTEGER, size_t, lower_bound),  //
                 (INTEGER, size_t, count)))

META_CLASS_IMPL(di::Node, Enumerator,
                ((STRING, name),             //
                 (INTEGER, ssize_t, value),  //
                 (INTEGER, bool, is_unsigned)))

META_CLASS_IMPL(di::Node, Inheritance,
                ((REF, di::Scope, scope),  //
                 (REF, di::Type, base),    //
                 (INTEGER, size_t, offset_in_bits)))

const StructureType& Inheritance::get_base_structure_type() const {
  return *dyn_cast<StructureType>(&get_base().strip_typedefs_and_qualifiers());
}

META_CLASS_IMPL(di::Node, Member,
                ((STRING, name),                     //
                 (REF, di::File, file),              //
                 (REF, di::Scope, scope),            //
                 (REF, di::Type, type),              //
                 (INTEGER, size_t, line),            //
                 (INTEGER, size_t, offset_in_bits),  //
                 (INTEGER, size_t, size_in_bits)))

std::string Member::get_pretty_name() const {
  return fmt::format("{}::{}", get_scope().get_pretty_name(), get_name());
}

Scope::~Scope() {
}

std::ostream& operator<<(std::ostream& os, const Language& language) {
  switch (language) {
    case Language::C:
      os << "C";
      break;
    case Language::C89:
      os << "C89";
      break;
    case Language::C99:
      os << "C99";
      break;
    case Language::C11:
      os << "C11";
      break;
    case Language::Cpp:
      os << "Cpp";
      break;
    case Language::Cpp03:
      os << "Cpp03";
      break;
    case Language::Cpp11:
      os << "Cpp11";
      break;
    case Language::Cpp14:
      os << "Cpp14";
      break;
    default:
      LOG_FATAL("Missing switch case {}", static_cast<int>(language));
      abort();
  }
  return os;
}

std::istream& operator>>(std::istream& is, std::optional<Language>& value) {
  std::string input;
  is >> input;
  if (input == "C") {
    value = Language::C;
  } else if (input == "C89") {
    value = Language::C89;
  } else if (input == "C99") {
    value = Language::C99;
  } else if (input == "C11") {
    value = Language::C11;
  } else if (input == "Cpp") {
    value = Language::Cpp;
  } else if (input == "Cpp03") {
    value = Language::Cpp03;
  } else if (input == "Cpp11") {
    value = Language::Cpp11;
  } else if (input == "Cpp14") {
    value = Language::Cpp14;
  } else {
    value = {};
  }
  return is;
}

META_CLASS_IMPL(di::Scope, GlobalOrBuiltin)

std::string GlobalOrBuiltin::get_pretty_name() const {
  return "";
}

bool File::is_unknown() const {
  return get_filename() == "?" && get_directory() == "?";
}

META_CLASS_IMPL(di::Scope, File,
                ((STRING, filename),  //
                 (STRING, directory)))

std::string File::get_pretty_name() const {
  return fmt::format("{}/{}", get_directory(), get_filename());
}

META_CLASS_IMPL(di::Scope, CompileUnit,
                ((REF, di::File, file),          //
                 (STRING, producer),             //
                 (INTEGER, Language, language),  //
                 (INTEGER, bool, is_optimized),  //
                 (INTEGER, size_t, runtime_version)))

std::string CompileUnit::get_pretty_name() const {
  return get_file().get_pretty_name();
}

META_CLASS_IMPL(di::Scope, Namespace,
                ((STRING, name),  //
                 (REF, di::Scope, scope)))

std::string Namespace::get_pretty_name() const {
  auto scope_name = get_scope().get_pretty_name();
  if (scope_name == "") {
    return get_name();
  } else {
    return fmt::format("{}::{}", scope_name, get_name());
  }
}

Type::~Type() {
}

const di::Type& Type::strip_typedefs_and_qualifiers() const {
  auto result = this;
  while (auto derived_type = dyn_cast<DerivedType>(result)) {
    switch (derived_type->get_tag()) {
      case DerivedKind::Typedef:
      case DerivedKind::Const:
      case DerivedKind::Restrict:
      case DerivedKind::Volatile:
        result = &derived_type->get_base_type();
        break;
      case DerivedKind::Pointer:
      case DerivedKind::Reference:
      case DerivedKind::RvalueReference:
      case DerivedKind::PtrToMemberType:
        return *result;
      default:
        LOG_FATAL("Missing switch case {}!", derived_type->get_tag());
        abort();
    }
  }
  return *result;
}

META_CLASS_IMPL(di::Type, VoidType)

std::string VoidType::get_pretty_name() const {
  return "void";
}

size_t VoidType::get_size_in_bits() const {
  return 0;
}

std::ostream& operator<<(std::ostream& os, const Encoding& encoding) {
  switch (encoding) {
    case Encoding::Unspecified:
      os << "Unspecified";
      break;
    case Encoding::Address:
      os << "Address";
      break;
    case Encoding::Boolean:
      os << "Boolean";
      break;
    case Encoding::Float:
      os << "Float";
      break;
    case Encoding::Signed:
      os << "Signed";
      break;
    case Encoding::Unsigned:
      os << "Unsigned";
      break;
    case Encoding::SignedChar:
      os << "SignedChar";
      break;
    case Encoding::UnsignedChar:
      os << "UnsignedChar";
      break;
    default:
      LOG_FATAL("Missing switch case {}", static_cast<int>(encoding));
      abort();
  }
  return os;
}

std::istream& operator>>(std::istream& is, std::optional<Encoding>& value) {
  std::string input;
  is >> input;
  if (input == "Unspecified") {
    value = Encoding::Unspecified;
  } else if (input == "Address") {
    value = Encoding::Address;
  } else if (input == "Boolean") {
    value = Encoding::Boolean;
  } else if (input == "Float") {
    value = Encoding::Float;
  } else if (input == "Signed") {
    value = Encoding::Signed;
  } else if (input == "Unsigned") {
    value = Encoding::Unsigned;
  } else if (input == "SignedChar") {
    value = Encoding::SignedChar;
  } else if (input == "UnsignedChar") {
    value = Encoding::UnsignedChar;
  } else {
    value = {};
  }
  return is;
}

META_CLASS_IMPL(di::Type, BasicType,
                ((STRING, name),                 //
                 (INTEGER, Encoding, encoding),  //
                 (INTEGER, size_t, size_in_bits)))

std::string BasicType::get_pretty_name() const {
  return get_name();
}

META_CLASS_IMPL(di::Type, StructureType,
                ((STRING, name),                          //
                 (STRING, identifier),                    //
                 (REF, di::File, file),                   //
                 (REF, di::Scope, scope),                 //
                 (INTEGER, size_t, line),                 //
                 (INTEGER, size_t, size_in_bits),         //
                 (TUPLE, di::Inheritance, base_classes),  //
                 (TUPLE, di::Subprogram, methods),        //
                 (TUPLE, di::Member, direct_members)))

std::string StructureType::get_pretty_name() const {
  auto scope_name = get_scope().get_pretty_name();
  auto name       = get_name() != "" ? get_name() : (get_identifier() != "" ? get_identifier() : "<unnamed struct>");
  if (scope_name == "") {
    return name;
  } else {
    return fmt::format("{}::{}", scope_name, name);
  }
}

const di::Inheritance* StructureType::find_inheritance(size_t offset_in_bits) const {
  for (const auto& inheritance : get_base_classes()) {
    if (inheritance.get_offset_in_bits() == offset_in_bits) {
      return &inheritance;
    }
  }
  return nullptr;
}

const di::Member* StructureType::find_member(size_t offset_in_bits) const {
  for (const auto& member : get_direct_members()) {
    if (member.get_offset_in_bits() <= offset_in_bits &&
        offset_in_bits < member.get_offset_in_bits() + member.get_type().get_size_in_bits()) {
      return &member;
    }
  }
  for (const auto& inheritance : get_base_classes()) {
    if (auto member =
            inheritance.get_base_structure_type().find_member(offset_in_bits - inheritance.get_offset_in_bits());
        member != nullptr) {
      return member;
    }
  }
  return nullptr;
}

META_CLASS_IMPL(di::Type, UnionType,
                ((STRING, name),                    //
                 (STRING, identifier),              //
                 (REF, di::File, file),             //
                 (REF, di::Scope, scope),           //
                 (INTEGER, size_t, line),           //
                 (INTEGER, size_t, size_in_bits),   //
                 (TUPLE, di::Subprogram, methods),  //
                 (TUPLE, di::Member, members)))

std::string UnionType::get_pretty_name() const {
  return get_name();
}

META_CLASS_IMPL(di::Type, ArrayType,
                ((REF, di::Type, base_type),       //
                 (INTEGER, size_t, size_in_bits),  //
                 (TUPLE, Integer, counts)))

size_t ArrayType::get_flattened_count() const {
  if (get_counts().size() == 0) {
    return 0;
  }
  auto result = 1;
  for (auto& count : get_counts()) {
    result *= dyn_cast<Integer>(&count)->get_data();
  }
  return result;
}

std::string ArrayType::get_pretty_name() const {
  std::vector<std::string> counts;
  std::transform(get_counts().begin(), get_counts().end(), std::back_inserter(counts),
                 [](auto& count) { return fmt::format("[{}]", meta::dyn_cast<meta::Integer>(&count)->get_data()); });
  return fmt::format("{}{}", get_base_type().get_pretty_name(), fmt::join(counts, ""));
}

META_CLASS_IMPL(di::Type, EnumerationType,
                ((STRING, name),                   //
                 (STRING, identifier),             //
                 (REF, di::File, file),            //
                 (REF, di::Scope, scope),          //
                 (INTEGER, size_t, line),          //
                 (INTEGER, size_t, size_in_bits),  //
                 (TUPLE, di::Node, elements)))

std::string EnumerationType::get_pretty_name() const {
  return get_name();
}

std::ostream& operator<<(std::ostream& os, const DerivedKind& kind) {
  switch (kind) {
    case DerivedKind::Typedef:
      os << "Typedef";
      break;
    case DerivedKind::Pointer:
      os << "Pointer";
      break;
    case DerivedKind::Reference:
      os << "Reference";
      break;
    case DerivedKind::RvalueReference:
      os << "RvalueReference";
      break;
    case DerivedKind::Const:
      os << "Const";
      break;
    case DerivedKind::Restrict:
      os << "Restrict";
      break;
    case DerivedKind::Volatile:
      os << "Volatile";
      break;
    case DerivedKind::PtrToMemberType:
      os << "PtrToMemberType";
      break;
    default:
      LOG_FATAL("Missing switch case {}", static_cast<int>(kind));
      abort();
  }
  return os;
}

std::istream& operator>>(std::istream& is, std::optional<DerivedKind>& value) {
  std::string input;
  is >> input;
  if (input == "Typedef") {
    value = DerivedKind::Typedef;
  } else if (input == "Pointer") {
    value = DerivedKind::Pointer;
  } else if (input == "Reference") {
    value = DerivedKind::Reference;
  } else if (input == "RvalueReference") {
    value = DerivedKind::RvalueReference;
  } else if (input == "Const") {
    value = DerivedKind::Const;
  } else if (input == "Restrict") {
    value = DerivedKind::Restrict;
  } else if (input == "Volatile") {
    value = DerivedKind::Volatile;
  } else if (input == "PtrToMemberType") {
    value = DerivedKind::PtrToMemberType;
  } else {
    value = {};
  }
  return is;
}

META_CLASS_IMPL(di::Type, DerivedType,
                ((STRING, name),                     //
                 (REF, di::File, file),              //
                 (REF, di::Scope, scope),            //
                 (REF, di::Type, base_type),         //
                 (INTEGER, DerivedKind, tag),        //
                 (INTEGER, size_t, line),            //
                 (INTEGER, size_t, offset_in_bits),  //
                 (INTEGER, size_t, size_in_bits)))

std::string DerivedType::get_pretty_name() const {
  if (get_name() != "") {
    auto scope_name = get_scope().get_pretty_name();
    if (get_scope().get_kind() != Kind::File && scope_name != "") {
      return fmt::format("{}::{}", get_scope().get_pretty_name(), get_name());
    } else {
      return fmt::format("{}", get_name());
    }
  } else {
    switch (get_tag()) {
      case DerivedKind::Pointer:
        return fmt::format("{}*", get_base_type().get_pretty_name());
      case DerivedKind::Const:
        return fmt::format("const {}", get_base_type().get_pretty_name());
      default:
        LOG_WARNING("Derived type with tag {} cannot be formatted!", get_tag());
        return "";
    }
  }
}

std::string SubroutineType::get_pretty_name() const {
  return "";  // TODO
}

META_CLASS_IMPL(di::Type, SubroutineType,
                ((REF, di::Type, return_type),  //
                 (TUPLE, di::Type, argument_types)))

LocalScope::~LocalScope() {
}

LexicalBlockBase::~LexicalBlockBase() {
}

META_CLASS_IMPL(di::LexicalBlockBase, LexicalBlock,
                ((REF, di::Scope, scope),  //
                 (REF, di::File, file),    //
                 (INTEGER, size_t, line),  //
                 (INTEGER, size_t, column)))

std::string LexicalBlock::get_pretty_name() const {
  return get_scope().get_pretty_name();
}

META_CLASS_IMPL(di::LexicalBlockBase, LexicalBlockFile,
                ((REF, di::Scope, scope),  //
                 (REF, di::File, file),    //
                 (INTEGER, size_t, discriminator)))

std::string LexicalBlockFile::get_pretty_name() const {
  return get_scope().get_pretty_name();
}

META_CLASS_IMPL(di::LocalScope, Subprogram,
                ((STRING, name),                   //
                 (STRING, linkage_name),           //
                 (REF, di::File, file),            //
                 (REF, di::Scope, scope),          //
                 (REF, di::SubroutineType, type),  //
                 (INTEGER, size_t, line)))

std::string Subprogram::get_pretty_name() const {
  if (get_scope().get_kind() != Kind::File) {
    return fmt::format("{}::{}", get_scope().get_pretty_name(), get_name());
  } else {
    return fmt::format("{}", get_name());
  }
}

META_CLASS_IMPL(meta::Node, Location,
                ((REF, di::LocalScope, scope),  //
                 (INTEGER, size_t, line),       //
                 (INTEGER, size_t, column)))

Variable::~Variable() {
}

META_CLASS_IMPL(Variable, LocalVariable,
                ((STRING, name),          //
                 (STRING, linkage_name),  //
                 (REF, Scope, scope),     //
                 (REF, File, file),       //
                 (REF, Type, type),       //
                 (INTEGER, size_t, line)))

META_CLASS_IMPL(Variable, GlobalVariable,
                ((STRING, name),             //
                 (STRING, linkage_name),     //
                 (REF, Scope, scope),        //
                 (REF, File, file),          //
                 (REF, Type, type),          //
                 (INTEGER, size_t, line),    //
                 (INTEGER, bool, is_local),  //
                 (INTEGER, bool, is_definition)))

}  // namespace di

Allocation::~Allocation() {
}

META_CLASS_IMPL(Allocation, StackAllocation,
                ((REF, di::LocalVariable, local_variable),  //
                 (REF, di::Location, location),             //
                 (OPTIONAL, Integer, count)))

META_CLASS_IMPL(Allocation, HeapAllocation,
                ((REF, di::Type, type),  //
                 (REF, di::Location, location)))

META_CLASS_IMPL(Allocation, GlobalAllocation, ((REF, di::GlobalVariable, global_variable)))

}  // namespace meta
