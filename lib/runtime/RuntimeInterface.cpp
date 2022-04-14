#include "RuntimeInterface.h"

#include "Runtime.h"
#include "allocator/Allocator.h"
#include "allocator/CallbackInterface.h"
#include "support/System.h"

namespace typeart::runtime::detail {
inline typeart_status query_type(const void* addr, int* type, size_t* count) {
  Runtime::getRecorder().incUsedInRequest(addr);

  auto pointer_info = Runtime::getPointerInfo(addr);
  if (pointer_info.has_value()) {
    return Runtime::getTypeResolution().getTypeInfo(addr, pointer_info.value(), type, count);
  }
  return TYPEART_UNKNOWN_ADDRESS;
}

inline typeart_status query_struct_layout(type_id_t type_id, typeart_struct_layout* struct_layout) {
  const typeart::StructTypeInfo* struct_info;
  typeart_status status = Runtime::getTypeResolution().getStructInfo(type_id, &struct_info);
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

}  // namespace typeart::runtime::detail

using namespace typeart;
using namespace typeart::runtime;

/**
 * Runtime interface implementation
 *
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
  auto result = detail::query_type(addr, type, count);
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

  auto pointer_info = std::optional<PointerInfo>{};

  pointer_info = Runtime::getPointerInfo(addr);
  if (pointer_info.has_value()) {
    const typeart::AllocationInfo* alloc_info;
    Runtime::getTypeResolution().getAllocationInfo(pointer_info->alloc_id, &alloc_info);
    *type         = alloc_info->type_id.value();
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
  return detail::query_struct_layout(type_id, struct_layout);
}

typeart_status typeart_resolve_type_id(int type_id, typeart_struct_layout* struct_layout) {
  auto guard = Runtime::scopeGuard();
  return detail::query_struct_layout(type_id, struct_layout);
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
    *file     = detail::string2char(source_loc->file);
    *function = detail::string2char(source_loc->function);
    *line     = detail::string2char(source_loc->line);

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
