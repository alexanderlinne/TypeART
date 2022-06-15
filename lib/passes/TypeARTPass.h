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

#ifndef _LIB_MUSTSUPPORTPASS_H
#define _LIB_MUSTSUPPORTPASS_H

#include "instrumentation/TypeARTInstrumentation.h"
#include "meta/Database.hpp"
#include "meta/LLVMMetadataConverter.hpp"

#include "llvm/Pass.h"

#include <memory>
#include <string>

namespace llvm {
class Module;
class Function;
class AnalysisUsage;
class Value;
class raw_ostream;
}  // namespace llvm

namespace typeart {
class TypeGenerator;

namespace analysis {
class MemInstFinder;
}  // namespace analysis

}  // namespace typeart

namespace typeart::pass {

class TypeArtPass : public llvm::ModulePass {
 private:
  std::unique_ptr<analysis::MemInstFinder> meminst_finder;

  std::string filename;
  std::unique_ptr<meta::Database> db;
  std::unique_ptr<meta::LLVMMetadataConverter> converter;
  std::unique_ptr<instrumentation::TypeArtInstrumentation> instrumentation;

 public:
  static char ID;  // used to identify pass

  TypeArtPass();
  bool doInitialization(llvm::Module&) override;
  bool runOnModule(llvm::Module&) override;
  bool runOnFunc(llvm::Function&);
  bool doFinalization(llvm::Module&) override;
  void getAnalysisUsage(llvm::AnalysisUsage&) const override;

 private:
  void addPreinitCall(llvm::Module&);
  void printStats(llvm::raw_ostream&);
};

}  // namespace typeart::pass

#endif
