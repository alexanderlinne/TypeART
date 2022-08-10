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

#include "ArgumentParser.h"

#include "runtime/allocator/Config.h"
#include "support/TypeUtil.h"

namespace typeart::instrumentation::hybrid {

ArgumentParser::ArgumentParser(llvm::Module& m, meta::LLVMMetadataConverter& converter)
    : instrumentation::ArgumentParser(), instr_helper(m), tracker_parser(m, converter), allocator_parser(m, converter) {
}

HeapArgList ArgumentParser::collectHeap(const MallocDataList& mallocs) {
  return tracker_parser.collectHeap(mallocs);
}

FreeArgList ArgumentParser::collectFree(const FreeDataList& frees) {
  return tracker_parser.collectFree(frees);
}

StackArgList ArgumentParser::collectStack(const AllocaDataList& allocs) {
  return allocator_parser.collectStack(allocs);
}

GlobalArgList ArgumentParser::collectGlobal(const GlobalDataList& globals) {
  return tracker_parser.collectGlobal(globals);
}

}  // namespace typeart::instrumentation::hybrid
