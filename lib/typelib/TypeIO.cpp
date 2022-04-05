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

#include "TypeIO.h"

#include "TypeDB.h"
#include "TypeDatabase.h"
#include "support/Logger.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/YAMLParser.h"
#include "llvm/Support/YAMLTraits.h"
#include "llvm/Support/raw_ostream.h"

#include <memory>
#include <system_error>
#include <vector>

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

struct AllocationInfoIO {
  typeart::allocation_id_t allocation_id;
  int type_id;

  llvm::Optional<size_t> count;

  // Offset from the pointer to the memory used by this allocation to the
  // pointer returned to the user.
  llvm::Optional<ptrdiff_t> base_ptr_offset;

  static AllocationInfoIO from(const typeart::AllocationInfo& info) {
    return {
        info.allocation_id,                                                                           //
        info.type_id,                                                                                 //
        info.count.has_value() ? llvm::Optional{info.count.value()} : llvm::None,                     //
        info.base_ptr_offset.has_value() ? llvm::Optional{info.base_ptr_offset.value()} : llvm::None  //
    };
  }

  typeart::AllocationInfo as_allocation_info() {
    return {
        allocation_id,                                                                         //
        type_id,                                                                               //
        count.hasValue() ? std::optional{count.getValue()} : std::nullopt,                     //
        base_ptr_offset.hasValue() ? std::optional{base_ptr_offset.getValue()} : std::nullopt  //
    };
  }
};

struct TypeFile {
  std::vector<AllocationInfoIO> allocation_info;
  std::vector<typeart::StructTypeInfo> struct_info;
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
    io.mapRequired("id", info.allocation_id);
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
struct llvm::yaml::MappingTraits<typeart::StructTypeInfo> {
  static void mapping(IO& io, typeart::StructTypeInfo& info) {
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

LLVM_YAML_IS_SEQUENCE_VECTOR(typeart::StructTypeInfo)

namespace typeart::io {

llvm::ErrorOr<bool> load(TypeDB* typeDB, const std::string& file) {
  using namespace llvm;
  ErrorOr<std::unique_ptr<MemoryBuffer>> memBuffer = MemoryBuffer::getFile(file);

  if (std::error_code error = memBuffer.getError(); error) {
    // TODO meaningful error handling/message
    LOG_WARNING("Warning while loading type file to " << file << ". Reason: " << error.message());
    return error;
  }

  typeDB->clear();

  yaml::Input in(memBuffer.get()->getMemBufferRef());
  TypeFile type_file;
  in >> type_file;

  for (auto& typeInfo : type_file.struct_info) {
    typeDB->registerStruct(typeInfo);
  }
  auto allocation_info = std::vector<AllocationInfo>{};
  allocation_info.reserve(type_file.allocation_info.size());
  std::transform(type_file.allocation_info.begin(), type_file.allocation_info.end(),
                 std::back_inserter(allocation_info), [](auto& info) { return info.as_allocation_info(); });
  typeDB->registerAllocations(std::move(allocation_info));

  return !in.error();
}

llvm::ErrorOr<bool> store(const TypeDB* typeDB, const std::string& file) {
  using namespace llvm;

  std::error_code error;
  raw_fd_ostream oss(StringRef(file), error, compat::open_flag());

  if (oss.has_error()) {
    LOG_WARNING("Warning while storing type file to " << file << ". Reason: " << error.message());
    return error;
  }

  auto types            = typeDB->getStructList();
  auto type_file        = TypeFile{};
  type_file.struct_info = typeDB->getStructList();

  auto allocation_info = typeDB->getAllocationInfo();
  type_file.allocation_info.reserve(allocation_info.size());
  std::transform(allocation_info.begin(), allocation_info.end(), std::back_inserter(type_file.allocation_info),
                 [](auto& info) { return AllocationInfoIO::from(info); });

  yaml::Output out(oss);
  out << type_file;
  return true;
}

}  // namespace typeart::io
