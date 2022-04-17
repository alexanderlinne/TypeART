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

#include "runtime/Runtime.hpp"

#include "AccessCountPrinter.h"
#include "db/Database.hpp"
#include "runtime/AccessCounter.hpp"
#include "runtime/tracker/Tracker.hpp"
#include "support/Logger.hpp"
#include "support/System.hpp"

#ifdef TYPEART_USE_ALLOCATOR
#include "allocator/Allocator.hpp"
#endif

#include <cstdlib>
#include <iostream>
#include <set>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace typeart::runtime {

static constexpr const char* defaultTypeFileName = "types.yaml";

class Runtime {
  // scope must be set to true before all other members are initialized.
  // This is achieved by adding this struct as the first member.
  struct Initializer {
    Initializer() {
      scope = 1;
    }

    void reset() const {
      scope = initial_scope;
    }

   private:
    size_t initial_scope = scope;
  };

  Initializer init;
  Database db{};
  Recorder recorder{};

 public:
  static thread_local size_t scope;

  static Runtime& get() {
    // As opposed to a global variable, a singleton + instantiation during
    // the first callback/query avoids some problems when
    // preloading (especially with MUST).
    static Runtime instance;
    return instance;
  }

  static Recorder& getRecorder() {
    return Runtime::get().recorder;
  }

  static Database& getDatabase() {
    return Runtime::get().db;
  }

 public:
  Runtime() : init() {
    LOG_TRACE("TypeART Runtime Trace");
    LOG_TRACE("*********************");

    auto loadTypes = [this](const std::string& file, std::error_code& ec) -> bool {
      auto database = Database::load(file);
      if (database.has_value()) {
        db = std::move(database).value();
        return true;
      }
      return false;
    };

    std::error_code error;
    // Try to load types from specified file first.
    // Then look at default location.
    const char* type_file = std::getenv("TYPEART_TYPE_FILE");
    if (type_file == nullptr || strcmp(type_file, "") == 0) {
      // FIXME Deprecated name
      type_file = std::getenv("TA_TYPE_FILE");
      if (type_file != nullptr) {
        LOG_WARNING("Use of deprecated env var TA_TYPE_FILE.");
      }
    }
    if (type_file != nullptr && strcmp(type_file, "") != 0) {
      if (!loadTypes(type_file, error)) {
        LOG_FATAL("Failed to load recorded types from TYPEART_TYPE_FILE=" << type_file
                                                                          << ". Reason: " << error.message());
        std::exit(EXIT_FAILURE);  // TODO: Error handling
      }
    } else {
      if (!loadTypes(defaultTypeFileName, error)) {
        LOG_WARNING(
            "No type file with default name \""
            << defaultTypeFileName
            << "\" in current directory. Using default built-in types only. To specify a different file, edit the "
               "TYPEART_TYPE_FILE environment variable. Reason: "
            << error.message());
      }
    }

    std::stringstream ss;
    const auto& typeList = db.getStructTypes();
    for (const auto& structInfo : typeList) {
      if (structInfo.isValid()) {
        ss << structInfo.name << ", ";
      }
    }
    recorder.incUDefTypes(typeList.size());
    LOG_INFO("Recorded types: " << ss.str());
    init.reset();
  }

  ~Runtime() {
    scope = 1;
    std::ostringstream stream;
    softcounter::serialize(recorder, stream);
    if (!stream.str().empty()) {
      // llvm::errs/LOG will crash with virtual call error
      std::cerr << stream.str();
    }
  }
};

thread_local size_t Runtime::scope = 0;

std::ostream& operator<<(std::ostream& os, const PointerInfo& pointer_info) {
  const auto name     = Runtime::getDatabase().getTypeName(pointer_info.type_id);
  const auto typeSize = Runtime::getDatabase().getTypeSize(pointer_info.type_id);
  os << pointer_info.type_id.value() << " " << name << " " << typeSize << " " << pointer_info.count << " ("
     << pointer_info.debug << ")";
  return os;
}

typeart_status getTypeInfoInternal(const void* baseAddr, size_t offset, const StructType& containerInfo,
                                   PointerInfo& result) {
  assert(offset < containerInfo.extent && "Something went wrong with the base address computation");

  typeart_status status;
  PointerInfo subtype_pointer_info;
  const StructType* structInfo = &containerInfo;
  size_t subTypeOffset;

  bool resolve{true};

  // Resolve type recursively, until the address matches exactly
  while (resolve) {
    status = getSubTypeInfo(baseAddr, offset, *structInfo, subtype_pointer_info, &subTypeOffset);

    if (status != TYPEART_OK) {
      return status;
    }

    baseAddr = subtype_pointer_info.base_addr;
    offset   = subTypeOffset;

    // Continue as long as there is a byte offset
    resolve = offset != 0;

    // Get layout of the nested struct
    if (resolve) {
      status = getStructType(subtype_pointer_info.type_id, &structInfo);
      if (status != TYPEART_OK) {
        return status;
      }
    }
  }
  result.type_id = subtype_pointer_info.type_id;
  result.count   = subtype_pointer_info.count;
  return TYPEART_OK;
}

typeart_status getContainingInfo(const void* addr, const PointerInfo& pointer_info, PointerInfo& result,
                                 size_t* offset);

typeart_status getPointerInfo(const void* addr, PointerInfo& result) {
  auto guard = ScopeGuard{};
  result     = {};
  getRecorder().incUsedInRequest(addr);
#ifdef TYPEART_USE_ALLOCATOR
  auto pointer_info = allocator::getPointerInfo(addr);
#else
  auto pointer_info = tracker::Tracker::get().getPointerInfo(addr);
#endif
  if (!pointer_info.has_value()) {
    getRecorder().incAddrMissing(addr);
    return TYPEART_UNKNOWN_ADDRESS;
  }
  result = pointer_info.value();

  // First, retrieve the containing type
  PointerInfo containing_pointer_info;
  size_t internalOffset;
  auto status = getContainingInfo(addr, result, containing_pointer_info, &internalOffset);
  if (status != TYPEART_OK) {
    return status;
  }

  // Check for exact address match
  if (internalOffset == 0) {
    result.type_id = containing_pointer_info.type_id;
    result.count   = containing_pointer_info.count;
    return TYPEART_OK;
  }

  if (Runtime::getDatabase().isBuiltinType(containing_pointer_info.type_id)) {
    // Address points to the middle of a builtin type
    return TYPEART_BAD_ALIGNMENT;
  }

  // Resolve struct recursively
  const auto* structInfo = Runtime::getDatabase().getStructType(containing_pointer_info.type_id);
  if (structInfo != nullptr) {
    const void* containingTypeAddr = static_cast<const int8_t*>(addr) - internalOffset;
    return getTypeInfoInternal(containingTypeAddr, internalOffset, *structInfo, result);
  }
  return TYPEART_INVALID_TYPE_ID;
}

typeart_status getContainingInfo(const void* addr, const PointerInfo& pointer_info, PointerInfo& result,
                                 size_t* offset) {
  result          = pointer_info;
  auto type_id    = pointer_info.type_id;
  size_t typeSize = Runtime::getDatabase().getTypeSize(type_id);

  // Check for exact match -> no further checks and offsets calculations needed
  if (pointer_info.base_addr == addr) {
    *offset = 0;
    return TYPEART_OK;
  }

  // The address points inside a known array
  const void* blockEnd = static_cast<const int8_t*>(pointer_info.base_addr) + pointer_info.count * typeSize;

  // Ensure that the given address is in bounds and points to the start of an element
  if (addr >= blockEnd) {
    const std::ptrdiff_t offset2base =
        static_cast<const uint8_t*>(addr) - static_cast<const uint8_t*>(pointer_info.base_addr);
    const auto oob_index = (offset2base / typeSize) - pointer_info.count + 1;
    LOG_WARNING("Out of bounds for the lookup: (" << addr << " " << pointer_info
                                                  << ") #Elements too far: " << oob_index);
    return TYPEART_UNKNOWN_ADDRESS;
  }

  assert(addr >= pointer_info.base_addr && "Error in base address computation");
  size_t addrDif = reinterpret_cast<size_t>(addr) - reinterpret_cast<size_t>(pointer_info.base_addr);

  // Offset of the pointer w.r.t. the start of the containing type
  size_t internalOffset = addrDif % typeSize;

  // Array index
  size_t typeOffset = addrDif / typeSize;
  size_t typeCount  = pointer_info.count - typeOffset;

  // Retrieve and return type information
  result.count = typeCount;
  *offset      = internalOffset;
  return TYPEART_OK;
}

typeart_status getContainingInfo(const void* addr, PointerInfo& result, size_t* offset) {
  auto guard = ScopeGuard{};
  result     = {};
  getRecorder().incUsedInRequest(addr);
#ifdef TYPEART_USE_ALLOCATOR
  auto pointer_info = allocator::getPointerInfo(addr);
#else
  auto pointer_info = tracker::Tracker::get().getPointerInfo(addr);
#endif
  if (!pointer_info.has_value()) {
    getRecorder().incAddrMissing(addr);
    return TYPEART_UNKNOWN_ADDRESS;
  }
  result = pointer_info.value();
  return getContainingInfo(addr, result, result, offset);
}

typeart_status getStructType(const void* addr, const StructType** structInfo) {
  auto guard = ScopeGuard{};
  PointerInfo pointer_info;
  auto status = getPointerInfo(addr, pointer_info);
  if (status != TYPEART_OK) {
    return status;
  }
  return getStructType(pointer_info.type_id, structInfo);
}

typeart_status getStructType(type_id_t type_id, const StructType** structInfo) {
  auto guard = ScopeGuard{};
  // Requested ID must correspond to a struct
  if (!Runtime::getDatabase().isStructType(type_id)) {
    return TYPEART_WRONG_KIND;
  }

  const auto* result = Runtime::getDatabase().getStructType(type_id);
  if (result != nullptr) {
    *structInfo = result;
    return TYPEART_OK;
  }
  return TYPEART_INVALID_TYPE_ID;
}

size_t getMemberIndex(typeart_struct_layout structInfo, size_t offset) {
  size_t n = structInfo.num_members;
  if (offset > structInfo.offsets[n - 1]) {
    return n - 1;
  }

  size_t i = 0;
  while (i < n - 1 && offset >= structInfo.offsets[i + 1]) {
    ++i;
  }
  return i;
}

typeart_status getSubTypeInfo(const void* baseAddr, size_t offset, typeart_struct_layout containerInfo,
                              PointerInfo& subtype_pointer_info, size_t* subTypeOffset) {
  if (offset >= containerInfo.extent) {
    return TYPEART_BAD_OFFSET;
  }

  // Get index of the struct member at the address
  size_t memberIndex = getMemberIndex(containerInfo, offset);

  int memberType = containerInfo.member_types[memberIndex];

  size_t baseOffset = containerInfo.offsets[memberIndex];
  assert(offset >= baseOffset && "Invalid offset values");

  size_t internalOffset   = offset - baseOffset;
  size_t typeSize         = Runtime::getDatabase().getTypeSize(memberType);
  size_t offsetInTypeSize = internalOffset / typeSize;
  size_t newOffset        = internalOffset % typeSize;

  // If newOffset != 0, the subtype cannot be atomic, i.e. must be a struct
  if (newOffset != 0) {
    if (Runtime::getDatabase().isReservedType(memberType)) {
      return TYPEART_BAD_ALIGNMENT;
    }
  }

  // Ensure that the array index is in bounds
  if (offsetInTypeSize >= containerInfo.count[memberIndex]) {
    // Address points to padding
    return TYPEART_BAD_ALIGNMENT;
  }

  subtype_pointer_info.base_addr = static_cast<const int8_t*>(baseAddr) + baseOffset;
  subtype_pointer_info.type_id   = memberType;
  subtype_pointer_info.count     = containerInfo.count[memberIndex] - offsetInTypeSize;
  subtype_pointer_info.debug     = nullptr;
  *subTypeOffset                 = newOffset;

  return TYPEART_OK;
}

typeart_status getSubTypeInfo(const void* baseAddr, size_t offset, const StructType& containerInfo,
                              PointerInfo& subtype_pointer_info, size_t* subTypeOffset) {
  return getSubTypeInfo(baseAddr, offset, static_cast<typeart_struct_layout>(containerInfo), subtype_pointer_info,
                        subTypeOffset);
}

const std::string& getTypeName(type_id_t type_id) {
  auto guard = ScopeGuard{};
  return Runtime::getDatabase().getTypeName(type_id);
}

size_t getTypeSize(type_id_t type_id) {
  auto guard = ScopeGuard{};
  return Runtime::getDatabase().getTypeSize(type_id);
}

bool isUnknown(type_id_t type_id) {
  auto guard = ScopeGuard{};
  return Runtime::getDatabase().isUnknown(type_id);
}

bool isValid(type_id_t type_id) {
  auto guard = ScopeGuard{};
  return Runtime::getDatabase().isValid(type_id);
}

bool isReservedType(type_id_t type_id) {
  auto guard = ScopeGuard{};
  return Runtime::getDatabase().isReservedType(type_id);
}

bool isBuiltinType(type_id_t type_id) {
  auto guard = ScopeGuard{};
  return Runtime::getDatabase().isBuiltinType(type_id);
}

bool isStructType(type_id_t type_id) {
  auto guard = ScopeGuard{};
  return Runtime::getDatabase().isStructType(type_id);
}

bool isUserDefinedType(type_id_t type_id) {
  auto guard = ScopeGuard{};
  return Runtime::getDatabase().isUserDefinedType(type_id);
}

bool isVectorType(type_id_t type_id) {
  auto guard = ScopeGuard{};
  return Runtime::getDatabase().isVectorType(type_id);
}

ScopeGuard::ScopeGuard() {
  Runtime::scope += 1;
}

ScopeGuard::~ScopeGuard() {
  Runtime::scope -= 1;
}

bool ScopeGuard::shouldTrack() const {
  return Runtime::scope <= 1;
}

Recorder& getRecorder() {
  return Runtime::getRecorder();
}

const AllocationInfo* getAllocationInfo(alloc_id_t alloc_id) {
  auto guard = ScopeGuard{};
  return Runtime::getDatabase().getAllocationInfo(alloc_id);
}

}  // namespace typeart::runtime

using namespace typeart;
using namespace typeart::runtime;

/**
 * Runtime interface implementation
 */

typeart_status typeart_get_pointer_info(const void* addr, typeart_pointer_info* result) {
  return getPointerInfo(addr, *reinterpret_cast<PointerInfo*>(result));
}

typeart_status typeart_get_containing_info(const void* addr, typeart_pointer_info* result, size_t* offset) {
  return getContainingInfo(addr, *reinterpret_cast<PointerInfo*>(result), offset);
}

typeart_status typeart_get_subtype_info(const void* base_addr, size_t offset, typeart_struct_layout container_layout,
                                        typeart_pointer_info* subtype_pointer_info, size_t* subtype_offset) {
  return getSubTypeInfo(base_addr, offset, container_layout, *reinterpret_cast<PointerInfo*>(subtype_pointer_info),
                        subtype_offset);
}

typeart_status typeart_resolve_type_addr(const void* addr, typeart_struct_layout* struct_layout) {
  const typeart::StructType* struct_info;
  typeart_status status = getStructType(addr, &struct_info);
  if (status == TYPEART_OK) {
    *struct_layout = *struct_info;
  }
  return status;
}

typeart_status typeart_resolve_type_id(int type_id, typeart_struct_layout* struct_layout) {
  const typeart::StructType* struct_info;
  typeart_status status = getStructType(type_id, &struct_info);
  if (status == TYPEART_OK) {
    *struct_layout = *struct_info;
  }
  return status;
}

typeart_status_t typeart_get_source_location(const void* addr, char** file, char** function, char** line) {
  auto guard = ScopeGuard{};

  auto source_loc = typeart::SourceLocation::create(addr);

  auto string2char = [](std::string_view src) {
    const size_t source_length = src.size() + 1;  // +1 for '\0'
    char* string_copy          = nullptr;

    // TODO track allocation
    string_copy = (char*)malloc(sizeof(char) * source_length);
    memcpy(string_copy, src.data(), source_length);
#ifndef TYPEART_USE_ALLOCATOR
    tracker::Tracker::get().onAlloc(string_copy, type_id_t{static_cast<type_id_t::value_type>(TYPEART_INT8)},
                                    source_length, __builtin_return_address(0));
#endif
    return string_copy;
  };

  if (source_loc) {
    *file     = string2char(source_loc->file);
    *function = string2char(source_loc->function);
    *line     = string2char(source_loc->line);

    if (*file == nullptr || *function == nullptr || *line == nullptr) {
      return TYPEART_ERROR;
    }

    return TYPEART_OK;
  }

  return TYPEART_UNKNOWN_ADDRESS;
}

const char* typeart_get_type_name(int type_id) {
  return getTypeName(type_id).c_str();
}

bool typeart_is_vector_type(int type_id) {
  return isVectorType(type_id);
}

bool typeart_is_valid_type(int type_id) {
  return isValid(type_id_t{type_id});
}

bool typeart_is_reserved_type(int type_id) {
  return isReservedType(type_id);
}

bool typeart_is_builtin_type(int type_id) {
  return isBuiltinType(type_id);
}

bool typeart_is_struct_type(int type_id) {
  return isStructType(type_id);
}

bool typeart_is_userdefined_type(int type_id) {
  return isUserDefinedType(type_id);
}

size_t typeart_get_type_size(int type_id) {
  return getTypeSize(type_id);
}
