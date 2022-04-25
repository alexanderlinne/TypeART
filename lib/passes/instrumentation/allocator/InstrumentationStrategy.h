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

#include "../../CommandLine.h"
#include "../TypeARTInstrumentation.h"
#include "../common/InstrumentationHelper.h"
#include "../common/TypeARTFunctions.h"
#include "../tracker/InstrumentationStrategy.h"

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Type.h>

namespace typeart::instrumentation::allocator {

class InstrumentationStrategy final : public instrumentation::InstrumentationStrategy {
  cl::InstrumentationMode mode;
  common::TypeArtFunctions type_art_functions;
  common::InstrumentationHelper instr_helper;
  tracker::InstrumentationStrategy tracker_instrumentation;

 public:
  InstrumentationStrategy(cl::InstrumentationMode mode, llvm::Module& m);
  size_t instrumentHeap(const HeapArgList& heap) override;
  size_t instrumentFree(const FreeArgList& frees) override;
  size_t instrumentStack(const StackArgList& stack) override;
  size_t instrumentGlobal(const GlobalArgList& globals) override;

 private:
  llvm::AllocaInst* createWrapperAlloca(llvm::AllocaInst* alloca, bool is_vla);
};

}  // namespace typeart::instrumentation::allocator
