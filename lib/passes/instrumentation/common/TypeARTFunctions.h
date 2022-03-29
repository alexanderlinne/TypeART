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

#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"

namespace typeart::instrumentation::common {

struct TypeArtFunctions {
  TypeArtFunctions() = default;
  TypeArtFunctions(llvm::Module& m);

  TypeArtFunctions(const TypeArtFunctions&) = default;

  llvm::Function* tracker_alloc        = nullptr;
  llvm::Function* tracker_alloc_global = nullptr;
  llvm::Function* tracker_alloc_stack  = nullptr;
  llvm::Function* tracker_free         = nullptr;
  llvm::Function* tracker_leave_scope  = nullptr;

  llvm::Function* tracker_alloc_omp        = nullptr;
  llvm::Function* tracker_alloc_stacks_omp = nullptr;
  llvm::Function* tracker_free_omp         = nullptr;
  llvm::Function* tracker_leave_scope_omp  = nullptr;

  llvm::Function* allocator_malloc = nullptr;
  llvm::Function* allocator__Znwm  = nullptr;
  llvm::Function* allocator__Znam  = nullptr;
};

}  // namespace typeart::instrumentation::common
