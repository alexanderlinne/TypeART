#include "meta/Meta.hpp"

#include "meta/Database.hpp"
#include "support/Logger.hpp"

#include <iostream>
#include <limits>
#include <queue>

namespace meta {

template <class T>
std::string serialize(T value) = delete;

template <>
std::string serialize<size_t>(size_t value) {
  return fmt::format("{}", value);
}

template <>
std::string serialize<ssize_t>(ssize_t value) {
  return fmt::format("{}", value);
}

template <>
std::string serialize<bool>(bool value) {
  if (value) {
    return "true";
  } else {
    return "false";
  }
}

template <>
std::string serialize<di::Language>(di::Language value) {
  std::ostringstream buffer;
  buffer << value;
  return buffer.str();
}

template <>
std::string serialize<di::Encoding>(di::Encoding value) {
  std::ostringstream buffer;
  buffer << value;
  return buffer.str();
}

template <>
std::string serialize<di::DerivedKind>(di::DerivedKind value) {
  std::ostringstream buffer;
  buffer << value;
  return buffer.str();
}

template <>
std::string serialize<std::vector<size_t>>(std::vector<size_t> value) {
  std::vector<std::string> strings;
  for (auto& v : value) {
    strings.push_back(::meta::serialize<size_t>(v));
  }
  return fmt::format("{}", fmt::join(strings, ","));
}

template <>
std::string serialize<std::optional<size_t>>(std::optional<size_t> value) {
  std::vector<std::string> strings;
  if (value.has_value()) {
    return ::meta::serialize<size_t>(value.value());
  } else {
    return "None";
  }
}

template <class T>
T deserialize(const std::string& str) = delete;

template <>
size_t deserialize<size_t>(const std::string& str) {
  return std::stoull(str);
}

template <>
ssize_t deserialize<ssize_t>(const std::string& str) {
  return std::stoll(str);
}

template <>
bool deserialize<bool>(const std::string& str) {
  if (str == "true") {
    return true;
  } else if (str == "false") {
    return false;
  } else {
    abort();
  }
}

template <>
di::Language deserialize<di::Language>(const std::string& str) {
  std::istringstream stream(str);
  std::optional<di::Language> result;
  stream >> result;
  if (!result.has_value()) {
    abort();
  }
  return result.value();
}

template <>
di::Encoding deserialize<di::Encoding>(const std::string& str) {
  std::istringstream stream(str);
  std::optional<di::Encoding> result;
  stream >> result;
  if (!result.has_value()) {
    abort();
  }
  return result.value();
}

template <>
di::DerivedKind deserialize<di::DerivedKind>(const std::string& str) {
  std::istringstream stream(str);
  std::optional<di::DerivedKind> result;
  stream >> result;
  if (!result.has_value()) {
    abort();
  }
  return result.value();
}

template <>
std::vector<size_t> deserialize<std::vector<size_t>>(const std::string& str) {
  std::string s = str;
  std::vector<size_t> result;
  size_t pos = 0;
  std::string token;
  while ((pos = s.find(",")) != std::string::npos) {
    token = s.substr(0, pos);
    result.push_back(::meta::deserialize<size_t>(token));
    s.erase(0, pos + 1);
  }
  result.push_back(::meta::deserialize<size_t>(s));
  return result;
}

template <>
std::optional<size_t> deserialize<std::optional<size_t>>(const std::string& str) {
  if (str == "None") {
    return {};
  } else {
    return {::meta::deserialize<size_t>(str)};
  }
}

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
    case Kind::String:
      return static_cast<const String&>(lhs).get_data() == static_cast<const String&>(rhs).get_data();
    default:
      return lhs.compare(rhs);
  }
}

std::unique_ptr<Meta> make_meta(Kind kind) {
  auto result = std::unique_ptr<Meta>{};
  switch (kind) {
    case Kind::String:
      result = std::make_unique<String>();
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

META_CLASS_IMPL(Meta, String,  //
                (0, ()),       //
                (0, ()))

TupleBase::~TupleBase() {
}

META_CLASS_IMPL(TupleBase, Tuple,  //
                (0, ()),           //
                (0, ()))

META_CLASS_IMPL(TupleBase, Optional,  //
                (0, ()),              //
                (0, ()))

Node::~Node() {
}

namespace di {

Node::~Node() {
}

META_CLASS_IMPL(di::Node, Subrange,          //
                (0, ()),                     //
                (2, ((size_t, lower_bound),  //
                     (size_t, count))))

META_CLASS_IMPL(di::Node, Enumerator,   //
                (1, ((STRING, name))),  //
                (2, ((ssize_t, value),  //
                     (bool, is_unsigned))))

META_CLASS_IMPL(di::Node, Inheritance,         //
                (2, ((REF, di::Scope, scope),  //
                     (REF, di::Type, base))),  //
                (1, ((size_t, offset_in_bits))))

const StructureType& Inheritance::get_base_structure_type() const {
  return *dyn_cast<StructureType>(&get_base().strip_typedefs_and_qualifiers());
}

META_CLASS_IMPL(di::Node, Member,               //
                (4, ((STRING, name),            //
                     (REF, di::File, file),     //
                     (REF, di::Scope, scope),   //
                     (REF, di::Type, type))),   //
                (3, ((size_t, line),            //
                     (size_t, offset_in_bits),  //
                     (size_t, size_in_bits))))

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

META_CLASS_IMPL(di::Scope, GlobalOrBuiltin,  //
                (0, ()),                     //
                (0, ()))

std::string GlobalOrBuiltin::get_pretty_name() const {
  return "";
}

bool File::is_unknown() const {
  return get_filename() == "?" && get_directory() == "?";
}

META_CLASS_IMPL(di::Scope, File,
                (2, ((STRING, filename),     //
                     (STRING, directory))),  //
                (0, ()))

std::string File::get_pretty_name() const {
  return fmt::format("{}/{}", get_directory(), get_filename());
}

META_CLASS_IMPL(di::Scope, CompileUnit,
                (2, ((REF, di::File, file),  //
                     (STRING, producer))),   //
                (3, ((Language, language),   //
                     (bool, is_optimized),   //
                     (size_t, runtime_version))))

std::string CompileUnit::get_pretty_name() const {
  return get_file().get_pretty_name();
}

META_CLASS_IMPL(di::Scope, Namespace,
                (2, ((STRING, name),             //
                     (REF, di::Scope, scope))),  //
                (0, ()))

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

META_CLASS_IMPL(di::Type, VoidType,  //
                (0, ()),             //
                (0, ()))

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

META_CLASS_IMPL(di::Type, BasicType,        //
                (1, ((STRING, name))),      //
                (2, ((Encoding, encoding),  //
                     (size_t, size_in_bits, override))))

std::string BasicType::get_pretty_name() const {
  return get_name();
}

META_CLASS_IMPL(di::Type, StructureType,
                (7, ((STRING, name),                          //
                     (STRING, identifier),                    //
                     (REF, di::File, file),                   //
                     (REF, di::Scope, scope),                 //
                     (TUPLE, di::Inheritance, base_classes),  //
                     (TUPLE, di::Subprogram, methods),        //
                     (TUPLE, di::Member, direct_members))),
                (2, ((size_t, line),                      //
                     (size_t, size_in_bits, override))))  //

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
                (6, ((STRING, name),                    //
                     (STRING, identifier),              //
                     (REF, di::File, file),             //
                     (REF, di::Scope, scope),           //
                     (TUPLE, di::Subprogram, methods),  //
                     (TUPLE, di::Member, members))),
                (2, ((size_t, line),                      //
                     (size_t, size_in_bits, override))))  //

std::string UnionType::get_pretty_name() const {
  return get_name();
}

META_CLASS_IMPL(di::Type, ArrayType,  //
                (1, ((REF, di::Type, base_type))),
                (2, ((size_t, size_in_bits, override),  //
                     (std::vector<size_t>, counts))))   //

size_t ArrayType::get_flattened_count() const {
  if (counts.size() == 0) {
    return 0;
  }
  auto result = 1;
  for (auto& count : counts) {
    result *= count;
  }
  return result;
}

std::string ArrayType::get_pretty_name() const {
  return fmt::format("{}{}", get_base_type().get_pretty_name(), fmt::join(counts, ""));
}

META_CLASS_IMPL(di::Type, EnumerationType,
                (5, ((STRING, name),           //
                     (STRING, identifier),     //
                     (REF, di::File, file),    //
                     (REF, di::Scope, scope),  //
                     (TUPLE, di::Node, elements))),
                (2, ((size_t, line),                      //
                     (size_t, size_in_bits, override))))  //

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
                (4, ((STRING, name),                //
                     (REF, di::File, file),         //
                     (REF, di::Scope, scope),       //
                     (REF, di::Type, base_type))),  //
                (4, ((DerivedKind, tag),            //
                     (size_t, line),                //
                     (size_t, offset_in_bits),      //
                     (size_t, size_in_bits, override))))

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
      case DerivedKind::Volatile:
        return fmt::format("volatile {}", get_base_type().get_pretty_name());
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
                (2, ((REF, di::Type, return_type),         //
                     (TUPLE, di::Type, argument_types))),  //
                (0, ()))

LocalScope::~LocalScope() {
}

LexicalBlockBase::~LexicalBlockBase() {
}

META_CLASS_IMPL(di::LexicalBlockBase, LexicalBlock,
                (2, ((REF, di::Scope, scope),            //
                     (REF, di::File, file, override))),  //
                (2, ((size_t, line),                     //
                     (size_t, column))))

std::string LexicalBlock::get_pretty_name() const {
  return get_scope().get_pretty_name();
}

META_CLASS_IMPL(di::LexicalBlockBase, LexicalBlockFile,
                (2, ((REF, di::Scope, scope),            //
                     (REF, di::File, file, override))),  //
                (1, ((size_t, discriminator))))

std::string LexicalBlockFile::get_pretty_name() const {
  return get_scope().get_pretty_name();
}

META_CLASS_IMPL(di::LocalScope, Subprogram,
                (5, ((STRING, name),                     //
                     (STRING, linkage_name),             //
                     (REF, di::File, file, override),    //
                     (REF, di::Scope, scope),            //
                     (REF, di::SubroutineType, type))),  //
                (1, ((size_t, line))))

std::string Subprogram::get_pretty_name() const {
  if (get_scope().get_kind() != Kind::File) {
    return fmt::format("{}::{}", get_scope().get_pretty_name(), get_name());
  } else {
    return fmt::format("{}", get_name());
  }
}

META_CLASS_IMPL(meta::Node, Location,                 //
                (1, ((REF, di::LocalScope, scope))),  //
                (2, ((size_t, line),                  //
                     (size_t, column))))

Variable::~Variable() {
}

META_CLASS_IMPL(Variable, LocalVariable,
                (5, ((STRING, name),          //
                     (STRING, linkage_name),  //
                     (REF, Scope, scope),     //
                     (REF, File, file),       //
                     (REF, Type, type))),     //
                (1, ((size_t, line))))

META_CLASS_IMPL(Variable, GlobalVariable,
                (5, ((STRING, name),          //
                     (STRING, linkage_name),  //
                     (REF, Scope, scope),     //
                     (REF, File, file),       //
                     (REF, Type, type))),     //
                (3, ((size_t, line),          //
                     (bool, is_local),        //
                     (bool, is_definition))))

}  // namespace di

Allocation::~Allocation() {
}

META_CLASS_IMPL(Allocation, StackAllocation,
                (2, ((REF, di::LocalVariable, local_variable),  //
                     (REF, di::Location, location))),           //
                (1, ((std::optional<size_t>, count))))

META_CLASS_IMPL(Allocation, HeapAllocation,
                (2, ((REF, di::Type, type, override),  //
                     (REF, di::Location, location))),  //
                (0, ()))

META_CLASS_IMPL(Allocation, GlobalAllocation,                       //
                (1, ((REF, di::GlobalVariable, global_variable))),  //
                (0, ()))

}  // namespace meta
