#include "RuntimeInterface.h"

#include "CallbackInterface.h"
#include "Runtime.h"
#include "allocator/Allocator.h"
#include "support/System.h"

namespace typeart::detail {
inline typeart_status query_type(const void* addr, int* type, size_t* count) {
  typeart::RuntimeSystem::get().recorder.incUsedInRequest(addr);

#if TYPEART_USE_ALLOCATOR
  auto allocation_info = allocator::getAllocationInfo(addr);
  if (allocation_info.has_value()) {
    return typeart::RuntimeSystem::get().typeResolution.getTypeInfo(addr, allocation_info->base_addr,
                                                                    allocation_info->pointer_info, type, count);
  }
#endif

  auto alloc = typeart::RuntimeSystem::get().allocTracker.findBaseAlloc(addr);
  if (alloc) {
    return typeart::RuntimeSystem::get().typeResolution.getTypeInfo(addr, alloc->first, alloc->second, type, count);
  }
  return TYPEART_UNKNOWN_ADDRESS;
}

inline typeart_status query_struct_layout(int id, typeart_struct_layout* struct_layout) {
  const typeart::StructTypeInfo* struct_info;
  typeart_status status = typeart::RuntimeSystem::get().typeResolution.getStructInfo(id, &struct_info);
  if (status == TYPEART_OK) {
    struct_layout->type_id      = struct_info->type_id;
    struct_layout->name         = struct_info->name.c_str();
    struct_layout->num_members  = struct_info->num_members;
    struct_layout->extent       = struct_info->extent;
    struct_layout->offsets      = &struct_info->offsets[0];
    struct_layout->member_types = &struct_info->member_types[0];
    struct_layout->count        = &struct_info->array_sizes[0];
  }
  return status;
}

char* string2char(std::string_view src) {
  const size_t source_length = src.size() + 1;  // +1 for '\0'
  char* string_copy          = nullptr;

#if TYPEART_USE_ALLOCATOR
  string_copy = (char*)typeart_allocator_malloc(TYPEART_INT8, source_length, sizeof(char) * source_length);
#else
  string_copy = (char*)malloc(sizeof(char) * source_length);
  if (string_copy == nullptr) {
    return nullptr;
  }
  const void* ret_addr = __builtin_return_address(0);
  typeart::RuntimeSystem::get().allocTracker.onAlloc(string_copy, TYPEART_INT8, source_length, ret_addr);
#endif

  memcpy(string_copy, src.data(), source_length);
  return string_copy;
};

}  // namespace typeart::detail

/**
 * Runtime interface implementation
 *
 */

typeart_status typeart_get_builtin_type(const void* addr, typeart::BuiltinType* type) {
  typeart::RTGuard guard;
  auto alloc = typeart::allocator::getAllocationInfo(addr);
  if (alloc.has_value()) {
    return typeart::RuntimeSystem::get().typeResolution.getBuiltinInfo(addr, alloc->pointer_info, type);
  }
  return TYPEART_UNKNOWN_ADDRESS;
}

typeart_status typeart_get_type(const void* addr, int* type, size_t* count) {
  typeart::RTGuard guard;
  auto result = typeart::detail::query_type(addr, type, count);
  if (result == TYPEART_UNKNOWN_ADDRESS) {
    typeart::RuntimeSystem::get().recorder.incAddrMissing(addr);
  }
  return result;
}

typeart_status typeart_get_type_length(const void* addr, size_t* count) {
  typeart::RTGuard guard;
  int type{0};
  return typeart_get_type(addr, &type, count);
}

typeart_status typeart_get_type_id(const void* addr, int* type_id) {
  typeart::RTGuard guard;
  size_t count{0};
  return typeart_get_type(addr, type_id, &count);
}

typeart_status typeart_get_containing_type(const void* addr, int* type, size_t* count, const void** base_address,
                                           size_t* offset) {
  typeart::RTGuard guard;

#if TYPEART_USE_ALLOCATOR
  auto alloc = typeart::allocator::getAllocationInfo(addr);
  if (alloc.has_value()) {
    *type         = alloc->pointer_info.typeId;
    *base_address = alloc->base_addr;
    return typeart::RuntimeSystem::get().typeResolution.getContainingTypeInfo(addr, alloc->base_addr,
                                                                              alloc->pointer_info, count, offset);
  }
#endif

  auto alloc2 = typeart::RuntimeSystem::get().allocTracker.findBaseAlloc(addr);
  if (alloc2) {
    *type         = alloc2->second.typeId;
    *base_address = alloc2->first;
    return typeart::RuntimeSystem::get().typeResolution.getContainingTypeInfo(addr, alloc2->first, alloc2->second,
                                                                              count, offset);
  }
  return TYPEART_UNKNOWN_ADDRESS;
}

typeart_status typeart_get_subtype(const void* base_addr, size_t offset, typeart_struct_layout container_layout,
                                   int* subtype_id, const void** subtype_base_addr, size_t* subtype_offset,
                                   size_t* subtype_count) {
  typeart::RTGuard guard;
  return typeart::RuntimeSystem::get().typeResolution.getSubTypeInfo(base_addr, offset, container_layout, subtype_id,
                                                                     subtype_base_addr, subtype_offset, subtype_count);
}

typeart_status typeart_resolve_type_addr(const void* addr, typeart_struct_layout* struct_layout) {
  typeart::RTGuard guard;
  int type_id{0};
  size_t size{0};
  auto status = typeart_get_type(addr, &type_id, &size);
  if (status != TYPEART_OK) {
    return status;
  }
  return typeart::detail::query_struct_layout(type_id, struct_layout);
}

typeart_status typeart_resolve_type_id(int type_id, typeart_struct_layout* struct_layout) {
  typeart::RTGuard guard;
  return typeart::detail::query_struct_layout(type_id, struct_layout);
}

typeart_status typeart_get_return_address(const void* addr, const void** return_addr) {
  typeart::RTGuard guard;

#if TYPEART_USE_ALLOCATOR
  auto alloc = typeart::allocator::getAllocationInfo(addr);
  if (alloc.has_value()) {
    *return_addr = alloc->pointer_info.debug;
    return TYPEART_OK;
  }
#endif

  auto alloc2 = typeart::RuntimeSystem::get().allocTracker.findBaseAlloc(addr);
  if (alloc2) {
    *return_addr = alloc2.getValue().second.debug;
    return TYPEART_OK;
  }
  *return_addr = nullptr;
  return TYPEART_UNKNOWN_ADDRESS;
}

typeart_status_t typeart_get_source_location(const void* addr, char** file, char** function, char** line) {
  using namespace typeart::detail;
  typeart::RTGuard guard;

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
  typeart::RTGuard guard;
  return typeart::RuntimeSystem::get().typeResolution.db().getTypeName(type_id).c_str();
}

bool typeart_is_vector_type(int type_id) {
  typeart::RTGuard guard;
  return typeart::RuntimeSystem::get().typeResolution.db().isVectorType(type_id);
}

bool typeart_is_valid_type(int type_id) {
  typeart::RTGuard guard;
  return typeart::RuntimeSystem::get().typeResolution.db().isValid(type_id);
}

bool typeart_is_reserved_type(int type_id) {
  typeart::RTGuard guard;
  return typeart::RuntimeSystem::get().typeResolution.db().isReservedType(type_id);
}

bool typeart_is_builtin_type(int type_id) {
  typeart::RTGuard guard;
  return typeart::RuntimeSystem::get().typeResolution.db().isBuiltinType(type_id);
}

bool typeart_is_struct_type(int type_id) {
  typeart::RTGuard guard;
  return typeart::RuntimeSystem::get().typeResolution.db().isStructType(type_id);
}

bool typeart_is_userdefined_type(int type_id) {
  typeart::RTGuard guard;
  return typeart::RuntimeSystem::get().typeResolution.db().isUserDefinedType(type_id);
}

size_t typeart_get_type_size(int type_id) {
  typeart::RTGuard guard;
  return typeart::RuntimeSystem::get().typeResolution.db().getTypeSize(type_id);
}
