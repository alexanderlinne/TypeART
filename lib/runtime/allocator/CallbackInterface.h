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

#pragma once

#include "meta/Meta.h"

#ifdef __cplusplus
#include <cstddef>
#else
#include <stddef.h>
#endif

// Callback function signatures invoked by the LLVM pass
#ifdef __cplusplus
extern "C" {
#endif

void* typeart_allocator_malloc(meta_id_value meta_id, size_t count, size_t size);
void* typeart_allocator_realloc(meta_id_value meta_id, size_t count, void* ptr, size_t new_size);
void* typeart_allocator__Znwm(meta_id_value meta_id, size_t count, size_t size);
void* typeart_allocator__Znam(meta_id_value meta_id, size_t count, size_t size);
int typeart_allocator_free(void*);

// Implemented in Allocator.cpp
void typeart_allocator_setup_main_stack(int argc, char** argv, char** envp);

#ifdef __cplusplus
}
#endif
