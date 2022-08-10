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

#include "InstrumentationStrategy.h"

#include "analysis/MemOpData.h"
#include "meta/Database.hpp"
#include "runtime/allocator/Config.h"
#include "support/Logger.hpp"
#include "support/OmpUtil.h"
#include "support/Util.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"

#include <fmt/format.h>
#include <string>

namespace llvm {
class Value;
}  // namespace llvm

namespace typeart::instrumentation::hybrid {

InstrumentationStrategy::InstrumentationStrategy(llvm::Module& m, bool instrument_lifetime)
    : instrumentation::InstrumentationStrategy(),
      type_art_functions(m),
      instr_helper(m),
      tracker_instrumentation(m, false),
      allocator_instrumentation(m, false),
      instrument_lifetime(instrument_lifetime) {
}

size_t InstrumentationStrategy::instrumentHeap(const HeapArgList& heap) {
  return tracker_instrumentation.instrumentHeap(heap);
}

size_t InstrumentationStrategy::instrumentFree(const FreeArgList& frees) {
  return tracker_instrumentation.instrumentFree(frees);
};

size_t InstrumentationStrategy::instrumentStack(const StackArgList& stack) {
  return allocator_instrumentation.instrumentStack(stack);
}

size_t InstrumentationStrategy::instrumentGlobal(const GlobalArgList& globals) {
  return tracker_instrumentation.instrumentGlobal(globals);
}

}  // namespace typeart::instrumentation::hybrid