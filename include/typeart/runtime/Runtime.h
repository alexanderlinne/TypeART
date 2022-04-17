// TypeART library
//
// Copyright (c) 2017-2021 TypeART Authors
// Distributed under the BSD 3-Clause license.
// (See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/BSD-3-Clause)
//
// Project home: https://github.com/tudasc/TypeART
//
// SPDX-License-Identifier: BSD-3-Clause
//

#pragma once

#include "../db/Types.h"

#ifdef __cplusplus

#include <cstddef>
#else

#include <stdbool.h>
#include <stddef.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct typeart_pointer_info_t {
  const void* base_addr;
  type_id_value type_id;
  size_t count;
  const void* return_addr;
} typeart_pointer_info;

typedef enum typeart_status_t {  // NOLINT
  TYPEART_OK,
  TYPEART_UNKNOWN_ADDRESS,
  TYPEART_BAD_ALIGNMENT,
  TYPEART_BAD_OFFSET,
  TYPEART_WRONG_KIND,
  TYPEART_INVALID_ALLOC_ID,
  TYPEART_INVALID_TYPE_ID,
  TYPEART_ERROR
} typeart_status;

typedef struct typeart_struct_layout_t {  // NOLINT
  type_id_value type_id;
  const char* name;
  size_t extent;
  size_t num_members;
  const size_t* offsets;
  const type_id_value* member_types;
  const size_t* count;
} typeart_struct_layout;

/**
 * Determines the pointer information at the given address.
 * For nested types with classes/structs, the containing type is resolved recursively, until an exact with the address
 * is found.
 *
 * Note that this function will always return the outermost type lining up with the address.
 * Given a pointer to the start of a struct, the returned type will therefore be that of the struct, not of the first
 * member.
 *
 * \param[in] addr The address.
 * \param[out] result The pointer info.
 *
 * \return A status code:
 *  - TYPEART_OK: The query was successful and the contents of type and count are valid.
 *  - TYPEART_UNKNOWN_ADDRESS: The given address is either not allocated, or was not correctly recorded by the runtime.
 *  - TYPEART_BAD_ALIGNMENT: The given address does not line up with the start of the atomic type at that location.
 *  - TYPEART_INVALID_ID: Encountered unregistered ID during lookup.
 */
typeart_status typeart_get_pointer_info(const void* addr, typeart_pointer_info* result);

/**
 * Determines the outermost type and array element count at the given address.
 * Unlike in typeart_get_pointer_info(), there is no further resolution of subtypes.
 * Instead, additional information about the position of the address within the containing type is returned.
 *
 * The starting address of the referenced array element can be deduced by computing `(size_t) addr - offset`.
 *
 * \param[in] addr The address.
 * \param[out] result The pointer_info.
 * \param[out] offset The byte offset within that buffer element.
 *
 * \return A status code. For an explanation of errors, refer to typeart_get_type().
 *
 */
typeart_status typeart_get_containing_info(const void* addr, typeart_pointer_info* result, size_t* offset);

/**
 * Determines the subtype at the given offset w.r.t. a base address and a corresponding containing type.
 * Note that if the subtype is itself a struct, you may have to call this function again.
 * If it returns with *subTypeOffset == 0, the address has been fully resolved.
 *
 * \param[in] baseAddr Pointer to the start of the containing type.
 * \param[in] offset Byte offset within the containing type.
 * \param[in] container_layout typeart_struct_layout corresponding to the containing type
 * \param[out] subtype_pointer_info The pointer_info for the subtype.
 * \param[out] subtype_offset Byte offset within the subtype.
 *
 * \return One of the following status codes:
 *  - TYPEART_OK: Success.
 *  - TYPEART_BAD_ALIGNMENT: Address corresponds to location inside an atomic type or padding.
 *  - TYPEART_BAD_OFFSET: The provided offset is invalid.
 */
typeart_status typeart_get_subtype_info(const void* base_addr, size_t offset, typeart_struct_layout container_layout,
                                        typeart_pointer_info* subtype_pointer_info, size_t* subtype_offset);

/**
 * Tries to return file, function and line of a memory address from the current process.
 * Needs (1) either llvm-symbolizer or addr2line to be installed, and (2) target code should be compiled debug
 * information for useful output. Note: file, function, line are allocated with malloc. They need to be free'd by the
 * caller.
 *
 * \param[in] addr The address.
 * \param[out] file The file where the address was created at.
 * \param[out] function The function where the address was created at.
 * \param[out] line The approximate line where the address was created at.
 *
 * \return One of the following status codes:
 *  - TYPEART_OK: Success.
 *  - TYPEART_UNKNOWN_ADDRESS: The given address is either not allocated, or was not recorded by the runtime.
 *  - TYPEART_ERROR: Memory could not be allocated.
 */
typeart_status typeart_get_source_location(const void* addr, char** file, char** function, char** line);

/**
 * Given an address, this function provides information about the corresponding struct type.
 * This is more expensive than the below version, since the pointer addr must be resolved.
 *
 * \param[in] addr The pointer address
 * \param[out] struct_layout Data layout of the struct.
 *
 * \return One of the following status codes:
 *  - TYPEART_OK: Success.
 *  - TYPEART_WRONG_KIND: ID does not correspond to a struct type.
 *  - TYPEART_UNKNOWN_ADDRESS: The given address is either not allocated, or was not correctly recorded by the runtime.
 *  - TYPEART_BAD_ALIGNMENT: The given address does not line up with the start of the atomic type at that location.
 *  - TYPEART_INVALID_ID: Encountered unregistered ID during lookup.
 */
typeart_status typeart_resolve_type_addr(const void* addr, typeart_struct_layout* struct_layout);

/**
 * Given a type ID, this function provides information about the corresponding struct type.
 *
 * \param[in] type_id The type ID.
 * \param[out] struct_layout Data layout of the struct.
 *
 * \return One of the following status codes:
 *  - TYPEART_OK: Success.
 *  - TYPEART_WRONG_KIND: ID does not correspond to a struct type.
 *  - TYPEART_INVALID_ID: ID is not valid.
 */
typeart_status typeart_resolve_type_id(int type_id, typeart_struct_layout* struct_layout);

/**
 * Returns the name of the type corresponding to the given type ID.
 * This can be used for debugging and error messages.
 *
 * \param[in] type_id The type ID.
 * \return The name of the type, or "typeart_unknown_struct" if the ID is unknown.
 */
const char* typeart_get_type_name(int type_id);

/**
 * Returns true if this is a valid type according to
 * e.g., a built-in type or a user-defined type,
 * see also TypeInterface.h
 *
 * \param[in] type_id The type ID.
 * \return true, false
 */
bool typeart_is_valid_type(int type_id);

/**
 * Returns true if the type ID is in the pre-determined reserved range,
 * see TypeInterface.h
 *
 * \param[in] type_id The type ID.
 * \return true, false
 */
bool typeart_is_reserved_type(int type_id);

/**
 * Returns true if the type ID is a built-in type,
 * see TypeInterface.h
 *
 * \param[in] type_id The type ID.
 * \return true, false
 */
bool typeart_is_builtin_type(int type_id);

/**
 * Returns true if the type ID is a structure type.
 * Note: This can be a user-defined struct or class, or a
 * LLVM vector type. Use the below queries for specifics.
 *
 * \param[in] type_id The type ID.
 * \return true, false
 */
bool typeart_is_struct_type(int type_id);

/**
 * Returns true if the type ID is a user-defined structure type
 * (struct, class etc.)
 *
 * \param[in] type_id The type ID.
 * \return true, false
 */
bool typeart_is_userdefined_type(int type_id);

/**
 * Returns true if the type ID is a LLVM SIMD vector type
 *
 * \param[in] type_id The type ID.
 * \return true, false
 */
bool typeart_is_vector_type(int type_id);

/**
 * Returns the byte size of the type behind the ID.
 *
 * \param[in] type_id The type ID.
 * \return size in bytes of the type
 */
size_t typeart_get_type_size(int type_id);

/**
 * Version string "major.minor(.patch)" of TypeART.
 *
 * \return version string
 */
const char* typeart_get_project_version();

/**
 * Short Git revision (length: 10) string of TypeART.
 * Char "+" is appended, if uncommitted changes were detected.
 * Returns "N/A" if revision couldn't be generated.
 *
 * \return revision string
 */
const char* typeart_get_git_revision();

/**
 * Version string "major.minor" of LLVM used to build TypeART.
 *
 * \return version string
 */
const char* typeart_get_llvm_version();

#ifdef __cplusplus
}
#endif
