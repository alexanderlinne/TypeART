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

#include "db/Database.hpp"

#include "support/Logger.hpp"

#include <iostream>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/YAMLParser.h>
#include <llvm/Support/YAMLTraits.h>
#include <llvm/Support/raw_ostream.h>
#include <utility>

using namespace llvm::yaml;

namespace compat {
auto open_flag() {
#if LLVM_VERSION_MAJOR < 13
  return llvm::sys::fs::OpenFlags::F_Text;
#else
  return llvm::sys::fs::OpenFlags::OF_Text;
#endif
}
}  // namespace compat

namespace typeart {

bool StructType::isValid() const {
  return type_id != type_id_t::invalid;
}

StructType::operator typeart_struct_layout() const {
  typeart_struct_layout struct_layout;
  struct_layout.type_id      = type_id.value();
  struct_layout.name         = name.c_str();
  struct_layout.num_members  = num_members;
  struct_layout.extent       = extent;
  struct_layout.offsets      = &offsets[0];
  struct_layout.member_types = reinterpret_cast<const type_id_t::value_type*>(&member_types[0]);
  struct_layout.count        = &array_sizes[0];
  return struct_layout;
}

const std::array<std::string, 11> Database::BuiltinNames = {
    "int8", "int16", "int32", "int64", "half", "float", "double", "float128", "x86_float80", "ppc_float128", "pointer"};

// TODO: Builtin ID changes lead tsto wrong type sizes/names
const std::array<size_t, 11> Database::BuiltinSizes = {1,  2,
                                                       4,  8,
                                                       2,  4,
                                                       8,  16,
                                                       16,  // TODO: Always correct?
                                                       16, sizeof(void*)};

const std::string Database::UnknownStructName{"typeart_unknown_struct"};

bool Database::isBuiltinType(type_id_t type_id) const {
  return type_id.value() >= TYPEART_INT8 && type_id.value() < TYPEART_NUM_VALID_IDS;
}

bool Database::isReservedType(type_id_t type_id) const {
  return type_id.value() < TYPEART_NUM_RESERVED_IDS;
}

bool Database::isStructType(type_id_t type_id) const {
  return type_id.value() >= TYPEART_NUM_RESERVED_IDS;
}

bool Database::isUserDefinedType(type_id_t type_id) const {
  const auto* structInfo = getStructType(type_id);
  return (structInfo != nullptr) &&
         ((static_cast<int>(structInfo->flag) & static_cast<int>(StructTypeFlag::USER_DEFINED)) != 0);
}

bool Database::isVectorType(type_id_t type_id) const {
  const auto* structInfo = getStructType(type_id);
  return (structInfo != nullptr) &&
         ((static_cast<int>(structInfo->flag) & static_cast<int>(StructTypeFlag::LLVM_VECTOR)) != 0);
}

bool Database::isValid(type_id_t type_id) const {
  if (isBuiltinType(type_id)) {
    return true;
  }
  return static_cast<type_id_t::value_type>(struct_types.size()) > type_id.value() &&
         struct_types[type_id.value()].type_id != type_id_t::invalid;
}

bool Database::isValid(alloc_id_t alloc_id) const {
  return alloc_id.value() > 0 && alloc_id.value() <= static_cast<alloc_id_t::value_type>(allocation_info.size());
}

void Database::registerStruct(const StructType& struct_type) {
  if (!struct_type.isValid()) {
    LOG_ERROR("Invalid type ID used for struct " << struct_type.name);
    return;
  }
  if (isValid(struct_type.type_id) || !isStructType(struct_type.type_id)) {
    if (isBuiltinType(struct_type.type_id)) {
      LOG_ERROR("Built-in type ID used for struct " << struct_type.name);
    } else if (isReservedType(struct_type.type_id)) {
      LOG_ERROR("Type ID is reserved for builtin types. Struct: " << struct_type.name);
    } else if (isUnknown(struct_type.type_id)) {
      LOG_ERROR("Type ID is reserved for unknown types. Struct: " << struct_type.name);
    } else {
      LOG_ERROR("Struct type ID already registered for " << struct_type.name << ". Conflicting struct is "
                                                         << getStructType(struct_type.type_id)->name);
    }
    return;
  }
  if (static_cast<type_id_t::value_type>(struct_types.size()) <= struct_type.type_id.value()) {
    struct_types.resize(struct_type.type_id.value() + 1);
  }
  struct_types[struct_type.type_id.value()] = struct_type;
}

alloc_id_t Database::getOrCreateAllocationId(type_id_t type_id, std::optional<size_t> count,
                                             std::optional<ptrdiff_t> base_ptr_offset) {
  auto it = std::find_if(allocation_info.begin(), allocation_info.end(), [&](auto& info) {
    return info.type_id == type_id && info.count == count && info.base_ptr_offset == base_ptr_offset;
  });
  if (it != allocation_info.end()) {
    return it->alloc_id;
  }
  AllocationInfo info;
  info.alloc_id        = static_cast<alloc_id_t::value_type>(allocation_info.size() + 1);
  info.type_id         = type_id;
  info.count           = count;
  info.base_ptr_offset = base_ptr_offset;
  allocation_info.push_back(std::move(info));
  return allocation_info.back().alloc_id;
}

void Database::registerAllocations(std::vector<AllocationInfo> allocations) {
  allocation_info = std::move(allocations);
  for (size_t i = 0; i < allocation_info.size(); ++i) {
    assert(allocation_info[i].alloc_id.value() - 1 == static_cast<alloc_id_t::value_type>(i));
  }
}

const std::string& Database::getTypeName(type_id_t type_id) const {
  if (isBuiltinType(type_id)) {
    return BuiltinNames[type_id.value()];
  }
  if (isStructType(type_id)) {
    const auto* structInfo = getStructType(type_id);
    if (structInfo != nullptr) {
      return structInfo->name;
    }
  }
  return UnknownStructName;
}

size_t Database::getTypeSize(type_id_t type_id) const {
  if (isReservedType(type_id)) {
    if (isBuiltinType(type_id)) {
      return BuiltinSizes[type_id.value()];
    }
    return 0;
  }

  const auto* structInfo = getStructType(type_id);
  if (structInfo != nullptr) {
    return structInfo->extent;
  }
  return 0;
}

const StructType* Database::getStructType(type_id_t type_id) const {
  if (isValid(type_id) && struct_types[type_id.value()].isValid()) {
    return &struct_types[type_id.value()];
  }
  return nullptr;
}

const std::vector<StructType>& Database::getStructTypes() const {
  return struct_types;
}

const AllocationInfo* Database::getAllocationInfo(alloc_id_t alloc_id) const {
  if (alloc_id.value() == 0 || alloc_id.value() > static_cast<alloc_id_t::value_type>(allocation_info.size())) {
    return nullptr;
  }
  return &allocation_info[alloc_id.value() - 1];
}

bool Database::isUnknown(type_id_t type_id) const {
  return type_id == type_id_t::unknown_type;
}

}  // namespace typeart

template <>
struct ScalarTraits<typeart::type_id_t> {
  static void output(const typeart::type_id_t& value, void* p, llvm::raw_ostream& out) {
    ScalarTraits<typeart::type_id_t::value_type>::output(value.value(), p, out);
  }
  static llvm::StringRef input(llvm::StringRef scalar, void* p, typeart::type_id_t& value) {
    typeart::type_id_t::value_type actual_value;
    auto result = ScalarTraits<typeart::type_id_t::value_type>::input(scalar, p, actual_value);
    value       = actual_value;
    return result;
  }
  static QuotingType mustQuote(llvm::StringRef scalar) {
    return ScalarTraits<typeart::type_id_t::value_type>::mustQuote(scalar);
  }
};

LLVM_YAML_IS_FLOW_SEQUENCE_VECTOR(typeart::type_id_t)

struct AllocationInfoIO {
  typeart::alloc_id_t::value_type alloc_id;
  typeart::type_id_t::value_type type_id;

  llvm::Optional<size_t> count;

  // Offset from the pointer to the memory used by this allocation to the
  // pointer returned to the user.
  llvm::Optional<ptrdiff_t> base_ptr_offset;

  static AllocationInfoIO from(const typeart::AllocationInfo& info) {
    return {
        info.alloc_id.value(),                                                                        //
        info.type_id.value(),                                                                         //
        info.count.has_value() ? llvm::Optional{info.count.value()} : llvm::None,                     //
        info.base_ptr_offset.has_value() ? llvm::Optional{info.base_ptr_offset.value()} : llvm::None  //
    };
  }

  typeart::AllocationInfo as_allocation_info() {
    return {
        alloc_id,                                                                              //
        type_id,                                                                               //
        count.hasValue() ? std::optional{count.getValue()} : std::nullopt,                     //
        base_ptr_offset.hasValue() ? std::optional{base_ptr_offset.getValue()} : std::nullopt  //
    };
  }
};

struct TypeFile {
  std::vector<AllocationInfoIO> allocation_info;
  std::vector<typeart::StructType> struct_info;
};

template <>
struct llvm::yaml::MappingTraits<TypeFile> {
  static void mapping(IO& io, TypeFile& info) {
    io.mapRequired("allocations", info.allocation_info);
    io.mapRequired("types", info.struct_info);
  }
};

template <>
struct llvm::yaml::MappingTraits<AllocationInfoIO> {
  static void mapping(IO& io, AllocationInfoIO& info) {
    io.mapRequired("id", info.alloc_id);
    io.mapRequired("type_id", info.type_id);
    io.mapOptional("count", info.count);
    io.mapOptional("base_ptr_offset", info.base_ptr_offset);
  }
};

LLVM_YAML_IS_SEQUENCE_VECTOR(AllocationInfoIO)

template <>
struct llvm::yaml::ScalarTraits<typeart::StructTypeFlag> {
  static void output(const typeart::StructTypeFlag& value, void*, llvm::raw_ostream& out) {
    out << static_cast<int>(value);
  }

  static StringRef input(StringRef scalar, void*, typeart::StructTypeFlag& value) {
    int flag{0};
    const auto result = scalar.getAsInteger(0, flag);
    if (result) {
      // Error result, assume user_defined:
      value = typeart::StructTypeFlag::USER_DEFINED;
    } else {
      value = static_cast<typeart::StructTypeFlag>(flag);
    }
    return StringRef();
  }

  // Determine if this scalar needs quotes.
  static QuotingType mustQuote(StringRef) {
    return QuotingType::None;
  }
};

template <>
struct llvm::yaml::MappingTraits<typeart::StructType> {
  static void mapping(IO& io, typeart::StructType& info) {
    io.mapRequired("id", info.type_id);
    io.mapRequired("name", info.name);
    io.mapRequired("extent", info.extent);
    io.mapRequired("member_count", info.num_members);
    io.mapRequired("offsets", info.offsets);
    io.mapRequired("types", info.member_types);
    io.mapRequired("sizes", info.array_sizes);
    io.mapRequired("flags", info.flag);
  }
};

LLVM_YAML_IS_SEQUENCE_VECTOR(typeart::StructType)

namespace typeart {

std::optional<Database> Database::load(const std::string& file) {
  using namespace llvm;
  ErrorOr<std::unique_ptr<MemoryBuffer>> memBuffer = MemoryBuffer::getFile(file);

  if (std::error_code error = memBuffer.getError(); error) {
    // TODO meaningful error handling/message
    LOG_WARNING("Warning while loading type file to " << file << ". Reason: " << error.message());
    return {};
  }

  yaml::Input in(memBuffer.get()->getMemBufferRef());
  TypeFile type_file;
  in >> type_file;

  Database db;
  for (const auto& typeInfo : type_file.struct_info) {
    db.registerStruct(typeInfo);
  }
  auto allocation_info = std::vector<AllocationInfo>{};
  allocation_info.reserve(type_file.allocation_info.size());
  std::transform(type_file.allocation_info.begin(), type_file.allocation_info.end(),
                 std::back_inserter(allocation_info), [](auto& info) { return info.as_allocation_info(); });
  db.registerAllocations(std::move(allocation_info));

  return db;
}

bool Database::store(const std::string& file) const {
  using namespace llvm;

  std::error_code error;
  raw_fd_ostream oss(StringRef(file), error, compat::open_flag());

  if (oss.has_error()) {
    LOG_WARNING("Warning while storing type file to " << file << ". Reason: " << error.message());
    return false;
  }

  auto type_file = TypeFile{};
  type_file.struct_info.reserve(struct_types.size());
  std::copy_if(struct_types.begin(), struct_types.end(), std::back_inserter(type_file.struct_info),
               [](auto& info) { return info.isValid(); });

  type_file.allocation_info.reserve(allocation_info.size());
  std::transform(allocation_info.begin(), allocation_info.end(), std::back_inserter(type_file.allocation_info),
                 [](auto& info) { return AllocationInfoIO::from(info); });

  yaml::Output out(oss);
  out << type_file;
  return true;
}

}  // namespace typeart