#include "db/Types.hpp"

#include "db/Database.hpp"
#include "support/Logger.hpp"

#include <limits>
#include <queue>

namespace typeart {

const alloc_id_t alloc_id_t::invalid = {};

std::ostream& operator<<(std::ostream& os, const alloc_id_t& alloc_id) {
  os << "alloc_id(" << alloc_id.value() << ")";
  return os;
}

const meta_id_t meta_id_t::invalid = {};

namespace meta {

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
    case Kind::String:
      os << "String";
      break;
    case Kind::Integer:
      os << "Integer";
      break;
    case Kind::Tuple:
      os << "Tuple";
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
    value = Kind::BasicType;
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
  } else if (input == "String") {
    value = Kind::String;
  } else if (input == "Integer") {
    value = Kind::Integer;
  } else if (input == "Tuple") {
    value = Kind::Tuple;
  } else {
    value = {};
  }
  return is;
}

bool Meta::contains_weak_refs() const {
  for (auto& ref : get_refs()) {
    if (ref.is_weak()) {
      return true;
    }
    if (auto meta = ref.get()) {
      if (meta->contains_weak_refs()) {
        return true;
      }
    } else {
      LOG_FATAL("Meta::contains_weak_refs called on Meta with nullptr Refs");
      abort();
    }
  }
  return false;
}

Meta::~Meta() {
}

std::unique_ptr<Meta> make_meta(Kind kind, std::vector<Ref<Meta>> refs) {
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
    default:
      LOG_FATAL("Missing switch case {}", kind);
      abort();
  }
  result->get_refs() = std::move(refs);
  return result;
}

namespace detail {
Meta* resolve_meta_id(meta_id_t id, Database& db) {
  return db.getMetaInfo(id);
}
}  // namespace detail

String::~String() {
}

Integer::~Integer() {
}

Tuple::~Tuple() {
}

Node::~Node() {
}

namespace di {

Node::~Node() {
}

Subrange::~Subrange() {
}

Enumerator::~Enumerator() {
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

GlobalOrBuiltin::~GlobalOrBuiltin() {
}

bool File::is_unknown() const {
  return get_filename() == "?" && get_directory() == "?";
}

File::~File() {
}

CompileUnit::~CompileUnit() {
}

Namespace::~Namespace() {
}

Type::~Type() {
}

const di::Type& Type::get_canonical_type() const {
  auto result = this;
  while (auto derived_type = dyn_cast<DerivedType>(result)) {
    switch (derived_type->get_tag()) {
      case DerivedKind::Typedef:
      case DerivedKind::Const:
      case DerivedKind::Restrict:
      case DerivedKind::Volatile:
        result = &derived_type->get_base_type();
        break;
      case DerivedKind::Member:
      case DerivedKind::Pointer:
      case DerivedKind::Reference:
      case DerivedKind::RvalueReference:
      case DerivedKind::Inheritance:
      case DerivedKind::PtrToMemberType:
        return *result;
      default:
        LOG_FATAL("Missing switch case {}!", derived_type->get_tag());
        abort();
    }
  }
  return *result;
}

std::string VoidType::get_pretty_name() const {
  return "void";
}

size_t VoidType::get_size_in_bits() const {
  return 0;
}

VoidType::~VoidType() {
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

std::string BasicType::get_pretty_name() const {
  return get_name();
}

BasicType::~BasicType() {
}

std::string StructureType::get_pretty_name() const {
  return get_name();
}

const di::DerivedType* StructureType::find_member(size_t offset_in_bits) const {
  for (const auto& e : get_elements()) {
    auto derived_type = dyn_cast<meta::di::DerivedType>(e.get());
    if (derived_type != nullptr && derived_type->get_tag() == DerivedKind::Member &&
        derived_type->get_offset_in_bits() <= offset_in_bits &&
        offset_in_bits < derived_type->get_offset_in_bits() + derived_type->get_size_in_bits()) {
      return derived_type;
    }
  }
  return nullptr;
}

StructureType::~StructureType() {
}

std::string UnionType::get_pretty_name() const {
  return get_name();
}

UnionType::~UnionType() {
}

size_t ArrayType::get_flattened_count() const {
  if (get_counts().size() == 0) {
    return 0;
  }
  auto result = 1;
  for (auto& count : get_counts()) {
    result *= dyn_cast<Integer>(count)->get_data();
  }
  return result;
}

std::string ArrayType::get_pretty_name() const {
  std::vector<std::string> counts;
  std::transform(get_counts().begin(), get_counts().end(), std::back_inserter(counts),
                 [](auto& count) { return fmt::format("[{}]", meta::dyn_cast<meta::Integer>(count)->get_data()); });
  return fmt::format("{}{}", get_base_type().get_pretty_name(), fmt::join(counts, ""));
}

ArrayType::~ArrayType() {
}

std::string EnumerationType::get_pretty_name() const {
  return get_name();
}

EnumerationType::~EnumerationType() {
}

std::ostream& operator<<(std::ostream& os, const DerivedKind& kind) {
  switch (kind) {
    case DerivedKind::Member:
      os << "Member";
      break;
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
    case DerivedKind::Inheritance:
      os << "Inheritance";
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
  if (input == "Member") {
    value = DerivedKind::Member;
  } else if (input == "Typedef") {
    value = DerivedKind::Typedef;
  } else if (input == "Pointer") {
    value = DerivedKind::Pointer;
  } else if (input == "Reference") {
    value = DerivedKind::Reference;
  } else if (input == "RvalueReference") {
    value = DerivedKind::RvalueReference;
  } else if (input == "Const") {
    value = DerivedKind::Const;
  } else if (input == "Inheritance") {
    value = DerivedKind::Inheritance;
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

std::string DerivedType::get_pretty_name() const {
  if (get_name() != "") {
    return get_name();
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
  }  // TODO handle pointer types etc. separately
}

DerivedType::~DerivedType() {
}

std::string SubroutineType::get_pretty_name() const {
  return "";  // TODO
}

SubroutineType::~SubroutineType() {
}

LocalScope::~LocalScope() {
}

LexicalBlockBase::~LexicalBlockBase() {
}

LexicalBlock::~LexicalBlock() {
}

LexicalBlockFile::~LexicalBlockFile() {
}

Subprogram::~Subprogram() {
}

Location::~Location() {
}

Variable::~Variable() {
}

LocalVariable::~LocalVariable() {
}

GlobalVariable::~GlobalVariable() {
}

}  // namespace di

Allocation::~Allocation() {
}

StackAllocation::~StackAllocation() {
}

HeapAllocation::~HeapAllocation() {
}

GlobalAllocation::~GlobalAllocation() {
}

}  // namespace meta

}  // namespace typeart
