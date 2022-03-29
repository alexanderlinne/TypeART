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

#include "TypeARTFunctions.h"

#include "InstrumentationHelper.h"
#include "support/Logger.h"

#include <string>

using namespace llvm;

namespace typeart::instrumentation::common {

llvm::Function* make_function(llvm::Module& m, llvm::StringRef name, llvm::Type* result_type,
                              llvm::ArrayRef<llvm::Type*> arg_types) {
  const auto addOptimizerAttributes = [&](llvm::Function* f) {
    for (Argument& arg : f->args()) {
      if (arg.getType()->isPointerTy()) {
        arg.addAttr(Attribute::NoCapture);
        arg.addAttr(Attribute::ReadOnly);
      }
    }
  };
  const auto setFunctionLinkageExternal = [](llvm::Function* f) { f->setLinkage(GlobalValue::ExternalLinkage); };
  const bool function_already_declared  = m.getFunction(name) != nullptr;
  const auto function_type              = llvm::FunctionType::get(result_type, arg_types, false);
  auto function_callee                  = m.getOrInsertFunction(name, function_type);
  Function* result                      = nullptr;
  if (function_already_declared) {
    result = dyn_cast<Function>(function_callee.getCallee()->stripPointerCasts());
  } else {
    result = dyn_cast<Function>(function_callee.getCallee());
    setFunctionLinkageExternal(result);
  }
  addOptimizerAttributes(result);
  return result;
}

llvm::Function* make_function(llvm::Module& m, llvm::StringRef basename, llvm::ArrayRef<llvm::Type*> args) {
  auto& c = m.getContext();
  return make_function(m, basename, Type::getVoidTy(c), args);
}

TypeArtFunctions::TypeArtFunctions(llvm::Module& m) {
  auto instrumentation_helper = InstrumentationHelper(m);
  auto alloc_arg_types        = instrumentation_helper.make_parameters(IType::ptr, IType::type_id, IType::extent);
  auto free_arg_types         = instrumentation_helper.make_parameters(IType::ptr);
  auto leavescope_arg_types   = instrumentation_helper.make_parameters(IType::stack_count);
  tracker_alloc               = make_function(m, "typeart_tracker_alloc", alloc_arg_types);
  tracker_alloc_stack         = make_function(m, "typeart_tracker_alloc_stack", alloc_arg_types);
  tracker_alloc_global        = make_function(m, "typeart_tracker_alloc_global", alloc_arg_types);
  tracker_free                = make_function(m, "typeart_tracker_free", free_arg_types);
  tracker_leave_scope         = make_function(m, "typeart_tracker_leave_scope", leavescope_arg_types);

  tracker_alloc_omp        = make_function(m, "typeart_tracker_alloc_omp", alloc_arg_types);
  tracker_alloc_stacks_omp = make_function(m, "typeart_tracker_alloc_stack_omp", alloc_arg_types);
  tracker_free_omp         = make_function(m, "typeart_tracker_free_omp", free_arg_types);
  tracker_leave_scope_omp  = make_function(m, "typeart_tracker_leave_scope_omp", leavescope_arg_types);

  auto malloc_arg_types = instrumentation_helper.make_parameters(IType::type_id, IType::extent, IType::extent);
  auto ptr_type         = instrumentation_helper.getTypeFor(IType::ptr);
  allocator_malloc      = make_function(m, "typeart_allocator_malloc", ptr_type, malloc_arg_types);
  allocator__Znwm       = make_function(m, "typeart_allocator__Znwm", ptr_type, malloc_arg_types);
  allocator__Znam       = make_function(m, "typeart_allocator__Znam", ptr_type, malloc_arg_types);
}

}  // namespace typeart::instrumentation::common
