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

Runtime::Runtime() : init(), typeResolution(db) {
  LOG_TRACE("TypeART Runtime Trace");
  LOG_TRACE("*********************");
  LOG_TRACE("Operation  Address   Type   Size   Count   (CallAddr)   Stack/Heap/Global");
  LOG_TRACE("-------------------------------------------------------------------------");

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

Runtime::~Runtime() {
  scope = 1;

  //  std::string stats;
  //  llvm::raw_string_ostream stream(stats);

  std::ostringstream stream;
  softcounter::serialize(recorder, stream);
  if (!stream.str().empty()) {
    // llvm::errs/LOG will crash with virtual call error
    std::cerr << stream.str();
  }
}

thread_local size_t Runtime::scope = 0;

std::optional<PointerInfo> Runtime::getPointerInfo(const void* addr) {
#ifdef TYPEART_USE_ALLOCATOR
  return allocator::getPointerInfo(addr);
#else
  return tracker::Tracker::get().getPointerInfo(addr);
#endif
}

std::string Runtime::toString(const void* addr, type_id_t type_id, size_t count, size_t typeSize,
                              const void* calledFrom) {
  return get().typeResolution.toString(addr, type_id, count, typeSize, calledFrom);
}

std::string Runtime::toString(const void* addr, type_id_t type_id, size_t count, const void* calledFrom) {
  return get().typeResolution.toString(addr, type_id, count, calledFrom);
}

std::string Runtime::toString(const void* addr, const PointerInfo& pointer_info) {
  return get().typeResolution.toString(addr, pointer_info);
}

inline typeart_status query_type(const void* addr, int* type, size_t* count) {
  Runtime::getRecorder().incUsedInRequest(addr);

  auto pointer_info = Runtime::getPointerInfo(addr);
  if (pointer_info.has_value()) {
    return Runtime::getTypeResolution().getTypeInfo(addr, pointer_info.value(), type, count);
  }
  return TYPEART_UNKNOWN_ADDRESS;
}

inline typeart_status query_struct_layout(type_id_t type_id, typeart_struct_layout* struct_layout) {
  const typeart::StructType* struct_info;
  typeart_status status = Runtime::getTypeResolution().getStructType(type_id, &struct_info);
  if (status == TYPEART_OK) {
    struct_layout->type_id      = struct_info->type_id.value();
    struct_layout->name         = struct_info->name.c_str();
    struct_layout->num_members  = struct_info->num_members;
    struct_layout->extent       = struct_info->extent;
    struct_layout->offsets      = &struct_info->offsets[0];
    struct_layout->member_types = reinterpret_cast<const type_id_t::value_type*>(&struct_info->member_types[0]);
    struct_layout->count        = &struct_info->array_sizes[0];
  }
  return status;
}

char* string2char(std::string_view src) {
  const size_t source_length = src.size() + 1;  // +1 for '\0'
  char* string_copy          = nullptr;

  // TODO track allocation
  string_copy = (char*)malloc(sizeof(char) * source_length);
  memcpy(string_copy, src.data(), source_length);
  return string_copy;
};

}  // namespace typeart::runtime

using namespace typeart;
using namespace typeart::runtime;

/**
 * Runtime interface implementation
 */

typeart_status typeart_get_builtin_type(const void* addr, BuiltinType* type) {
  auto guard = Runtime::scopeGuard();

  auto pointer_info = Runtime::getPointerInfo(addr);
  if (pointer_info.has_value()) {
    return Runtime::getTypeResolution().getBuiltinInfo(addr, pointer_info.value(), type);
  }
  return TYPEART_UNKNOWN_ADDRESS;
}

typeart_status typeart_get_type(const void* addr, int* type, size_t* count) {
  auto guard  = Runtime::scopeGuard();
  auto result = query_type(addr, type, count);
  if (result == TYPEART_UNKNOWN_ADDRESS) {
    Runtime::getRecorder().incAddrMissing(addr);
  }
  return result;
}

typeart_status typeart_get_type_length(const void* addr, size_t* count) {
  auto guard = Runtime::scopeGuard();
  int type{0};
  return typeart_get_type(addr, &type, count);
}

typeart_status typeart_get_type_id(const void* addr, int* type_id) {
  auto guard = Runtime::scopeGuard();
  size_t count{0};
  return typeart_get_type(addr, type_id, &count);
}

typeart_status typeart_get_containing_type(const void* addr, int* type, size_t* count, const void** base_address,
                                           size_t* offset) {
  auto guard = Runtime::scopeGuard();

  auto pointer_info = Runtime::getPointerInfo(addr);
  if (pointer_info.has_value()) {
    *type         = pointer_info->type_id.value();
    *base_address = pointer_info->base_addr;
    return Runtime::getTypeResolution().getContainingTypeInfo(addr, pointer_info.value(), count, offset);
  }
  return TYPEART_UNKNOWN_ADDRESS;
}

typeart_status typeart_get_subtype(const void* base_addr, size_t offset, typeart_struct_layout container_layout,
                                   int* subtype_id, const void** subtype_base_addr, size_t* subtype_offset,
                                   size_t* subtype_count) {
  auto guard = Runtime::scopeGuard();
  return Runtime::getTypeResolution().getSubTypeInfo(base_addr, offset, container_layout, subtype_id, subtype_base_addr,
                                                     subtype_offset, subtype_count);
}

typeart_status typeart_resolve_type_addr(const void* addr, typeart_struct_layout* struct_layout) {
  auto guard = Runtime::scopeGuard();
  type_id_t::value_type type_id{0};
  size_t size{0};
  auto status = typeart_get_type(addr, &type_id, &size);
  if (status != TYPEART_OK) {
    return status;
  }
  return typeart_resolve_type_id(type_id, struct_layout);
}

typeart_status typeart_resolve_type_id(int type_id, typeart_struct_layout* struct_layout) {
  auto guard = Runtime::scopeGuard();
  return query_struct_layout(type_id, struct_layout);
}

typeart_status typeart_get_return_address(const void* addr, const void** return_addr) {
  auto guard = Runtime::scopeGuard();

  auto pointer_info = Runtime::getPointerInfo(addr);
  if (pointer_info) {
    *return_addr = pointer_info->debug;
    return TYPEART_OK;
  }
  *return_addr = nullptr;
  return TYPEART_UNKNOWN_ADDRESS;
}

typeart_status_t typeart_get_source_location(const void* addr, char** file, char** function, char** line) {
  auto guard = Runtime::scopeGuard();

  auto source_loc = typeart::SourceLocation::create(addr);

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
  auto guard = Runtime::scopeGuard();
  return Runtime::getTypeResolution().getTypeName(type_id).c_str();
}

bool typeart_is_vector_type(int type_id) {
  auto guard = Runtime::scopeGuard();
  return Runtime::getTypeResolution().isVectorType(type_id);
}

bool typeart_is_valid_type(int type_id) {
  auto guard = Runtime::scopeGuard();
  return Runtime::getTypeResolution().isValid(type_id);
}

bool typeart_is_reserved_type(int type_id) {
  auto guard = Runtime::scopeGuard();
  return Runtime::getTypeResolution().isReservedType(type_id);
}

bool typeart_is_builtin_type(int type_id) {
  auto guard = Runtime::scopeGuard();
  return Runtime::getTypeResolution().isBuiltinType(type_id);
}

bool typeart_is_struct_type(int type_id) {
  auto guard = Runtime::scopeGuard();
  return Runtime::getTypeResolution().isStructType(type_id);
}

bool typeart_is_userdefined_type(int type_id) {
  auto guard = Runtime::scopeGuard();
  return Runtime::getTypeResolution().isUserDefinedType(type_id);
}

size_t typeart_get_type_size(int type_id) {
  auto guard = Runtime::scopeGuard();
  return Runtime::getTypeResolution().getTypeSize(type_id);
}
