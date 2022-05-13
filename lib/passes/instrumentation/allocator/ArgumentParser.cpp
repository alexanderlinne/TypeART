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

namespace typeart::instrumentation::allocator {

ArgumentParser::ArgumentParser(llvm::Module& m, TypeGenerator* tm)
    : instrumentation::ArgumentParser(), type_m(tm), instr_helper(m), tracker_parser(m, tm) {
}

HeapArgList ArgumentParser::collectHeap(const MallocDataList& mallocs) {
  return tracker_parser.collectHeap(mallocs);
}

FreeArgList ArgumentParser::collectFree(const FreeDataList& frees) {
  return {};
}

StackArgList ArgumentParser::collectStack(const AllocaDataList& allocs) {
  return tracker_parser.collectStack(allocs);
}

GlobalArgList ArgumentParser::collectGlobal(const GlobalDataList& globals) {
  return tracker_parser.collectGlobal(globals);
}

}  // namespace typeart::instrumentation::allocator
