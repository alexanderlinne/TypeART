#include "db/Types.hpp"

#include "db/Database.hpp"

#include <limits>

namespace typeart {

const type_id_t type_id_t::unknown_type = {};
const type_id_t type_id_t::invalid      = {std::numeric_limits<value_type>::max()};

std::ostream& operator<<(std::ostream& os, const type_id_t& alloc_id) {
  os << "type_id(" << alloc_id.value() << ")";
  return os;
}

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
    case Kind::BasicType:
      os << "BasicType";
      break;
    case Kind::CompositeType:
      os << "CompositeType";
      break;
    case Kind::DerivedType:
      os << "DerivedType";
      break;
    case Kind::SubroutineType:
      os << "SubrosineType";
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
    case Kind::Tuple:
      os << "Tuple";
      break;
    default:
      abort();
  }
  return os;
}

Meta::~Meta() {
}

namespace detail {
Meta* resolve_meta_id(meta_id_t id, Database& db) {
  return db.getMetaInfo(id);
}
}  // namespace detail

String::~String() {
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

File::~File() {
}

CompileUnit::~CompileUnit() {
}

Namespace::~Namespace() {
}

Type::~Type() {
}

BasicType::~BasicType() {
}

CompositeType::~CompositeType() {
}

DerivedType::~DerivedType() {
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
