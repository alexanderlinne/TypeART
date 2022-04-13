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

#include "TypeARTPass.h"

#include "CommandLine.h"
#include "instrumentation/allocator/ArgumentParser.h"
#include "instrumentation/allocator/InstrumentationStrategy.h"
#include "instrumentation/tracker/ArgumentParser.h"
#include "instrumentation/tracker/InstrumentationStrategy.h"
#include "support/Logger.h"
#include "support/Table.h"
#include "typegen/TypeGenerator.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"

#include <cassert>
#include <cstddef>
#include <sstream>
#include <string>
#include <utility>

#define DEBUG_TYPE "typeart"

static llvm::RegisterPass<typeart::pass::TypeArtPass> msp("typeart", "TypeArt type instrumentation sanitizer", false,
                                                          false);

ALWAYS_ENABLED_STATISTIC(NumInstrumentedMallocs, "Number of instrumented mallocs");
ALWAYS_ENABLED_STATISTIC(NumInstrumentedFrees, "Number of instrumented frees");
ALWAYS_ENABLED_STATISTIC(NumInstrumentedAlloca, "Number of instrumented (stack) allocas");
ALWAYS_ENABLED_STATISTIC(NumInstrumentedGlobal, "Number of instrumented globals");

namespace typeart::pass {

// Used by LLVM pass manager to identify passes in memory
char TypeArtPass::ID = 0;

TypeArtPass::TypeArtPass() : llvm::ModulePass(ID) {
  meminst_finder = analysis::create_finder(cl::getMemInstFinderConfig());
  llvm::EnableStatistics(false);
}

void TypeArtPass::getAnalysisUsage(llvm::AnalysisUsage& info) const {
}

bool TypeArtPass::doInitialization(llvm::Module& m) {
  const auto type_filepath = cl::getTypeFilepath();
  typeManager              = make_typegen(type_filepath);

  LOG_DEBUG("Propagating type infos.");
  const auto [loaded, error] = typeManager->load();
  if (loaded) {
    LOG_DEBUG("Existing type configuration successfully loaded from " << type_filepath);
  } else {
    LOG_DEBUG("No valid existing type configuration found: " << type_filepath << ". Reason: " << error.message());
  }

#ifdef TYPEART_USE_ALLOCATOR
  auto parser   = std::make_unique<instrumentation::allocator::ArgumentParser>(m, typeManager.get());
  auto strategy = std::make_unique<instrumentation::allocator::InstrumentationStrategy>(m);
#else
  auto parser = std::make_unique<instrumentation::tracker::ArgumentParser>(m, typeManager.get());
  auto strategy =
      std::make_unique<instrumentation::tracker::InstrumentationStrategy>(m, cl::getInstrumentStackLifetime());
#endif

  instrumentation = std::make_unique<instrumentation::TypeArtInstrumentation>(std::move(parser), std::move(strategy));

  return true;
}

bool TypeArtPass::runOnModule(llvm::Module& m) {
  meminst_finder->runOnModule(m);

  bool instrumented_global{false};
  if (cl::getInstrumentGlobal()) {
    const auto& globalsList = meminst_finder->getModuleGlobals();
    if (!globalsList.empty()) {
      const auto global_count = instrumentation->handleGlobal(globalsList);
      NumInstrumentedGlobal += global_count;
      instrumented_global = global_count > 0;
    }
  }

  const auto instrumented_function = llvm::count_if(m.functions(), [&](auto& f) { return runOnFunc(f); }) > 0;
  return instrumented_function || instrumented_global;
}

bool TypeArtPass::runOnFunc(llvm::Function& f) {
  using namespace typeart;

#ifdef TYPEART_USE_ALLOCATOR
  if (f.getName().equals("main")) {
    addPreinitCall(*f.getParent());
  }
#endif

  if (f.isDeclaration() || f.getName().startswith("typeart")) {
    return false;
  }

  if (!meminst_finder->hasFunctionData(f)) {
    LOG_WARNING("No allocation data could be retrieved for function: " << f.getName());
    return false;
  }

  LOG_DEBUG("Running on function: " << f.getName())

  bool mod{false};

  llvm::SmallDenseMap<llvm::BasicBlock*, size_t> allocCounts;

  const auto& fData   = meminst_finder->getFunctionData(f);
  const auto& mallocs = fData.mallocs;
  const auto& allocas = fData.allocas;
  const auto& frees   = fData.frees;

  if (cl::getInstrumentHeap()) {
    const auto heap_count = instrumentation->handleHeap(mallocs);
    const auto free_count = instrumentation->handleFree(frees);
    NumInstrumentedMallocs += heap_count;
    NumInstrumentedFrees += free_count;
    mod |= heap_count > 0 || free_count > 0;
  }

  if (cl::getInstrumentStack()) {
    const auto stack_count = instrumentation->handleStack(allocas);
    NumInstrumentedAlloca += stack_count;
    mod |= stack_count > 0;
  }

  return mod;
}  // namespace pass

bool TypeArtPass::doFinalization(llvm::Module&) {
  /*
   * Persist the accumulated type definition information for this module.
   */
  LOG_DEBUG("Writing type file to " << cl::getTypeFilepath());

  const auto [stored, error] = typeManager->store();
  if (stored) {
    LOG_DEBUG("Success!");
  } else {
    LOG_FATAL("Failed writing type config to " << cl::getTypeFilepath() << ". Reason: " << error.message());
  }
  if (cl::getPrintStats()) {
    auto& out = llvm::errs();
    printStats(out);
  }
  return false;
}

void TypeArtPass::addPreinitCall(llvm::Module& m) {
  auto& ctx          = m.getContext();
  auto string_arr_ty = llvm::Type::getInt8PtrTy(ctx)->getPointerTo();
  auto function_ty   = llvm::FunctionType::get(llvm::Type::getVoidTy(ctx),
                                               {llvm::Type::getInt32Ty(ctx), string_arr_ty, string_arr_ty}, false);
  auto preinit_function =
      llvm::Function::Create(function_ty, llvm::Function::ExternalLinkage, "typeart_allocator_setup_main_stack", m);
  m.getOrInsertGlobal("typeart_preinit", function_ty->getPointerTo());
  auto preinit_global = m.getNamedGlobal("typeart_preinit");
  preinit_global->setInitializer(preinit_function);
  preinit_global->setLinkage(llvm::GlobalValue::InternalLinkage);
  preinit_global->setAlignment(llvm::MaybeAlign(8));
  preinit_global->setSection(".preinit_array");
  llvm::appendToUsed(m, {preinit_global});
}

void TypeArtPass::printStats(llvm::raw_ostream& out) {
  meminst_finder->printStats(out);

  Table stats("TypeArtPass [Heap & Stack]");
  stats.wrap_header = true;
  stats.put(Row::make("Malloc", NumInstrumentedMallocs.getValue()));
  stats.put(Row::make("Free", NumInstrumentedFrees.getValue()));
  stats.put(Row::make("Alloca", NumInstrumentedAlloca.getValue()));
  stats.put(Row::make("Global", NumInstrumentedGlobal.getValue()));

  std::ostringstream stream;
  stats.print(stream);
  out << stream.str();
}

}  // namespace typeart::pass

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

static void registerClangPass(const llvm::PassManagerBuilder&, llvm::legacy::PassManagerBase& PM) {
  PM.add(new typeart::pass::TypeArtPass());
}
static llvm::RegisterStandardPasses RegisterClangPass(llvm::PassManagerBuilder::EP_OptimizerLast, registerClangPass);
