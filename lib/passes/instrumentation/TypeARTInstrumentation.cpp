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

#include "TypeARTInstrumentation.h"

#include <utility>

namespace typeart::instrumentation {
TypeArtInstrumentation::TypeArtInstrumentation(std::unique_ptr<ArgumentParser> parser,
                                               std::unique_ptr<InstrumentationStrategy> strategy)
    : parser(std::move(parser)), strategy(std::move(strategy)) {
}

size_t TypeArtInstrumentation::handleHeap(const MallocDataList& mallocs) {
  if (mallocs.empty()) {
    return 0;
  }
  return strategy->instrumentHeap(parser->collectHeap(mallocs));
}

size_t TypeArtInstrumentation::handleFree(const FreeDataList& frees) {
  if (frees.empty()) {
    return 0;
  }
  return strategy->instrumentFree(parser->collectFree(frees));
}

size_t TypeArtInstrumentation::handleStack(const AllocaDataList& allocas) {
  if (allocas.empty()) {
    return 0;
  }
  return strategy->instrumentStack(parser->collectStack(allocas));
}

size_t TypeArtInstrumentation::handleGlobal(const GlobalDataList& globals) {
  if (globals.empty()) {
    return 0;
  }
  return strategy->instrumentGlobal(parser->collectGlobal(globals));
}

}  // namespace typeart::instrumentation