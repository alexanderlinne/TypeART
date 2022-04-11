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

#ifdef __cplusplus
}
#endif
