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

#include "../TypeARTInstrumentation.h"
#include "../common/InstrumentationHelper.h"
#include "../common/TypeARTFunctions.h"

namespace typeart::instrumentation::tracker {

class InstrumentationStrategy final : public instrumentation::InstrumentationStrategy {
  llvm::Module* module;
  common::TypeArtFunctions type_art_functions;
  common::InstrumentationHelper instr_helper;
  bool instrument_lifetime{false};

 public:
  InstrumentationStrategy(llvm::Module& m, bool instrument_lifetime);
  size_t instrumentHeap(const HeapArgList& heap) override;
  size_t instrumentFree(const FreeArgList& frees) override;
  size_t instrumentStack(const StackArgList& stack) override;
  size_t instrumentGlobal(const GlobalArgList& globals) override;
};

}  // namespace typeart::instrumentation::tracker
