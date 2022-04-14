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

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/IRBuilder.h"

#include <cstddef>
#include <functional>
#include <map>
#include <string>

namespace typeart::instrumentation::common {

enum class IType {
  ptr,          // Type for passing a pointer to the runtime
  type_id,      // Type for identifying a type
  extent,       // Type for identifying an array length
  stack_count,  // Type for identifying a count of stack alloca instructions
  alloc_id,
};

class InstrumentationHelper {
  llvm::Module* module{nullptr};

 public:
  InstrumentationHelper();
  InstrumentationHelper(llvm::Module& m);

  template <typename... Types>
  llvm::SmallVector<llvm::Type*, 8> make_parameters(Types... args) {
    static_assert((std::is_same_v<IType, Types> && ...));
    return {getTypeFor(args)...};
  }

  llvm::Type* getTypeFor(IType id);
  llvm::ConstantInt* getConstantFor(IType id, size_t val = 0);
  const std::map<std::string, llvm::Function*>& getFunctionMap() const;

  virtual ~InstrumentationHelper();
};

}  // namespace typeart::instrumentation::common

namespace typeart::instrumentation {
using IType = common::IType;
}
