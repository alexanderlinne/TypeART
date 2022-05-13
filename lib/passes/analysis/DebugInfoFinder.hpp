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

#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>

namespace typeart::analysis {

llvm::DbgDeclareInst* findDbgDeclareInstFor(llvm::AllocaInst* alloca);
llvm::SmallVector<llvm::DIType*, 4> findDITypes(llvm::CallBase& call);

}  // namespace typeart::analysis
