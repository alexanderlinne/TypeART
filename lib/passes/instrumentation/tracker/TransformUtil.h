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

#ifndef TYPEART_TRANSFORMUTIL_H
#define TYPEART_TRANSFORMUTIL_H

#include "../common/InstrumentationHelper.h"
#include "../common/TypeARTFunctions.h"
#include "support/OmpUtil.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/EscapeEnumerator.h"

namespace typeart::instrumentation {

struct StackCounter {
  using StackOpCounter = llvm::SmallDenseMap<llvm::BasicBlock*, size_t>;
  llvm::Function* f;
  common::InstrumentationHelper* instr_helper;
  common::TypeArtFunctions type_art_functions;

  StackCounter(llvm::Function* f, common::InstrumentationHelper* instr, common::TypeArtFunctions* type_art_functions)
      : f(f), instr_helper(instr), type_art_functions(*type_art_functions) {
  }

  void addStackHandling(StackOpCounter& allocCounts) const {
    using namespace llvm;
    IRBuilder<> CBuilder(f->getEntryBlock().getFirstNonPHI());
    auto* counter = CBuilder.CreateAlloca(instr_helper->getTypeFor(IType::stack_count), nullptr, "__ta_alloca_counter");
    CBuilder.CreateStore(instr_helper->getConstantFor(IType::stack_count), counter);

    // In each basic block: counter =+ num_alloca (in BB)
    for (auto data : allocCounts) {
      IRBuilder<> IRB(data.first->getTerminator());
      auto* load_counter = IRB.CreateLoad(instr_helper->getTypeFor(IType::stack_count), counter);
      Value* increment_counter =
          IRB.CreateAdd(instr_helper->getConstantFor(IType::stack_count, data.second), load_counter);
      IRB.CreateStore(increment_counter, counter);
    }

    // Find return instructions:
    // if(counter > 0) call runtime for stack cleanup
    const auto callback = util::omp::isOmpContext(f) ? type_art_functions.tracker_leave_scope_omp
                                                     : type_art_functions.tracker_leave_scope;

    EscapeEnumerator ee(*f);
    while (IRBuilder<>* irb = ee.Next()) {
      auto* I            = &(*irb->GetInsertPoint());
      auto* counter_load = irb->CreateLoad(instr_helper->getTypeFor(IType::stack_count), counter, "__ta_counter_load");

      const auto all_preds_have_counter = llvm::all_of(
          llvm::predecessors(I->getParent()), [&allocCounts](const auto* bb) { return allocCounts.count(bb) > 0; });
      if (all_preds_have_counter) {
        auto* cond = irb->CreateICmpNE(counter_load, instr_helper->getConstantFor(IType::stack_count), "__ta_cond");
        auto* then_term = SplitBlockAndInsertIfThen(cond, I, false);
        irb->SetInsertPoint(then_term);
      }

      irb->CreateCall(callback, ArrayRef<Value*>{counter_load});
    }
  }
};

}  // namespace typeart::instrumentation

#endif  // TYPEART_TRANSFORMUTIL_H
