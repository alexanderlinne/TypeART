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

#ifndef TYPEART_CALLBACKINTERFACE_H
#define TYPEART_CALLBACKINTERFACE_H

#ifdef __cplusplus
#include <cstddef>
#else
#include <stddef.h>
#endif

// Callback function signatures invoked by the LLVM pass
#ifdef __cplusplus
extern "C" {
#endif

void* typeart_allocator_malloc(int type_id, size_t count, size_t size);
void* typeart_allocator__Znwm(int type_id, size_t count, size_t size);
void* typeart_allocator__Znam(int type_id, size_t count, size_t size);
int typeart_allocator_free(void*);

void typeart_tracker_alloc(const void* addr, int type_id, size_t count);
void typeart_tracker_alloc_global(const void* addr, int type_id, size_t count);
void typeart_tracker_free(const void* addr);

void typeart_tracker_alloc_stack(const void* addr, int type_id, size_t count);
void typeart_tracker_leave_scope(int alloca_count);

// Called from OpenMP context
void typeart_tracker_alloc_omp(const void* addr, int type_id, size_t count);
void typeart_tracker_free_omp(const void* addr);
void typeart_tracker_alloc_stack_omp(const void* addr, int type_id, size_t count);
void typeart_tracker_leave_scope_omp(int alloca_count);

#ifdef __cplusplus
}
#endif

#endif  // TYPEART_CALLBACKINTERFACE_H
