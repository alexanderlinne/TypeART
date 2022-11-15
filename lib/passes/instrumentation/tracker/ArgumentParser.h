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
#include "analysis/MemOpData.h"
#include "meta/Database.hpp"
#include "meta/LLVMMetadataConverter.hpp"

namespace typeart::instrumentation::tracker {

class ArgumentParser final : public instrumentation::ArgumentParser {
  llvm::Module* module;
  common::InstrumentationHelper instr_helper;
  meta::LLVMMetadataConverter* converter;

 public:
  ArgumentParser(llvm::Module& m, meta::LLVMMetadataConverter& converter);
  HeapArgList collectHeap(const MallocDataList& mallocs) override;
  FreeArgList collectFree(const FreeDataList& frees) override;
  StackArgList collectStack(const AllocaDataList& allocs) override;
  GlobalArgList collectGlobal(const GlobalDataList& globals) override;
};
}  // namespace typeart::instrumentation::tracker
