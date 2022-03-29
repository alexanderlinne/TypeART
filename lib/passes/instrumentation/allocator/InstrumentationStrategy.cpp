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

#include "InstrumentationStrategy.h"

#include "analysis/MemOpData.h"
#include "runtime/allocator/Config.h"
#include "support/Logger.h"
#include "support/OmpUtil.h"
#include "support/Util.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"

#include <fmt/format.h>
#include <string>

namespace llvm {
class Value;
}  // namespace llvm

namespace typeart::instrumentation::allocator {

InstrumentationStrategy::InstrumentationStrategy(llvm::Module& m)
    : instrumentation::InstrumentationStrategy(), type_art_functions(m), instr_helper(m), tracker_instrumentation(m) {
}

size_t InstrumentationStrategy::instrumentHeap(const HeapArgList& heap) {
  for (const auto& [malloc, args] : heap) {
    auto malloc_call = llvm::dyn_cast<llvm::CallBase>(args.get_as<llvm::Instruction>(ArgMap::ID::pointer));
    llvm::IRBuilder<> builder(malloc_call);
    auto type_id   = args.get_value(ArgMap::ID::type_id);
    auto type_size = args.get_value(ArgMap::ID::type_size);
    bool single_byte_type{false};
    if (auto const_int = llvm::dyn_cast<llvm::ConstantInt>(type_size)) {
      single_byte_type = const_int->equalsInt(1);
    }
    llvm::Value* element_count;
    switch (malloc.kind) {
      case MemOpKind::AlignedAllocLike:
        [[fallthrough]];
      case MemOpKind::NewLike:
        [[fallthrough]];
      case MemOpKind::MallocLike: {
        element_count = args.lookup(ArgMap::ID::element_count);
        if (element_count == nullptr) {
          auto bytes    = args.get_value(ArgMap::ID::byte_count);
          element_count = single_byte_type ? bytes : builder.CreateUDiv(bytes, type_size);
        }
        break;
      }
      case MemOpKind::CallocLike: {
        if (malloc.primary == nullptr) {
          auto elems    = args.get_value(ArgMap::ID::element_count);
          element_count = builder.CreateMul(elems, type_size);
        } else {
          element_count = args.get_value(ArgMap::ID::element_count);
        }
        break;
      }
      case MemOpKind::ReallocLike: {
        auto mArg     = args.get_value(ArgMap::ID::byte_count);
        element_count = single_byte_type ? mArg : builder.CreateUDiv(mArg, type_size);
        break;
      }
      default:
        LOG_ERROR("Unknown malloc kind. Not instrumenting. " << util::dump(*malloc_call));
        continue;
    }

    auto function_name = malloc_call->getCalledFunction()->getName();
    if (function_name == "malloc") {
      auto size           = malloc_call->getArgOperand(0);
      auto typeart_malloc = builder.CreateCall(type_art_functions.allocator_malloc, {type_id, element_count, size});
      malloc_call->replaceAllUsesWith(typeart_malloc);
      malloc_call->eraseFromParent();
    } else if (function_name == "_Znwm") {
      auto size          = malloc_call->getArgOperand(0);
      auto typeart__Znwm = builder.CreateCall(type_art_functions.allocator__Znwm, {type_id, element_count, size});
      malloc_call->replaceAllUsesWith(typeart__Znwm);
      malloc_call->eraseFromParent();
    } else if (function_name == "_Znam") {
      auto size          = malloc_call->getArgOperand(0);
      auto typeart__Znam = builder.CreateCall(type_art_functions.allocator__Znam, {type_id, element_count, size});
      malloc_call->replaceAllUsesWith(typeart__Znam);
      malloc_call->eraseFromParent();
    } else {
      malloc_call->print(llvm::errs());
      fprintf(stderr, "\nUnknown malloc function %s\n", function_name.str().c_str());
      abort();
    }
  }
  return heap.size();
}

size_t InstrumentationStrategy::instrumentFree(const FreeArgList& frees) {
  return 0;
};

std::string typeNameFor(llvm::Type* ty) {
  if (ty->isPointerTy()) {
    return fmt::format("{}*", typeNameFor(llvm::dyn_cast<llvm::PointerType>(ty)->getElementType()));
  } else if (ty->isStructTy()) {
    return ty->getStructName();
  } else if (ty->isIntegerTy()) {
    auto int_ty = llvm::dyn_cast<llvm::IntegerType>(ty);
    return fmt::format("i{}", int_ty->getBitWidth());
  } else if (ty->isFloatTy()) {
    return "float";
  } else if (ty->isDoubleTy()) {
    return "double";
  } else if (ty->isArrayTy()) {
    auto arr_ty = llvm::dyn_cast<llvm::ArrayType>(ty);
    return fmt::format("[{} x {}]", arr_ty->getNumElements(), typeNameFor(arr_ty->getElementType()));
  } else if (ty->isFunctionTy()) {
    auto fn_ty = llvm::dyn_cast<llvm::FunctionType>(ty);
    auto param = std::string("");
    for (auto& arg : fn_ty->params()) {
      param += typeNameFor(arg);
      param += ", ";
    }
    return fmt::format("{}({})", typeNameFor(fn_ty->getReturnType()), param);
  } else if (ty->isVoidTy()) {
    return "void";
  } else {
    ty->print(llvm::errs());
    llvm::errs() << "\n";
  }
  assert(false);
}

size_t InstrumentationStrategy::instrumentStack(const StackArgList& stack) {
  namespace config = typeart::allocator::config::stack;
  for (const auto& [sdata, args] : stack) {
    auto* alloca = args.get_as<llvm::AllocaInst>(ArgMap::ID::pointer);
    auto& ctx    = alloca->getContext();

    // Create a struct data type for the stack allocation
    auto type           = (llvm::Type*)nullptr;
    auto allocated_type = alloca->getAllocatedType();
    auto name           = std::string{"Typeart_Wrapper_"} + typeNameFor(allocated_type);
    type                = alloca->getModule()->getTypeByName(name);
    if (type == nullptr) {
      type = llvm::StructType::create(
          {instr_helper.getTypeFor(IType::type_id), instr_helper.getTypeFor(IType::extent), allocated_type}, name);
    }

    llvm::IRBuilder<> IRB(alloca->getNextNode());

    auto typeIdConst    = args.get_value(ArgMap::ID::type_id);
    auto numElementsVal = args.get_value(ArgMap::ID::element_count);

    auto wrapper_alloca = IRB.CreateAlloca(type);

    auto byte_size          = alloca->getModule()->getDataLayout().getTypeAllocSize(type).getFixedSize();
    auto required_alignment = config::alignment_for(byte_size);
    auto alignment          = alloca->getAlignment();
    wrapper_alloca->setAlignment(llvm::MaybeAlign{alignment >= required_alignment ? alignment : required_alignment});

    auto offset_wrapper = IRB.CreateBitCast(wrapper_alloca, llvm::Type::getInt8PtrTy(ctx));
    offset_wrapper      = IRB.CreateConstGEP1_64(offset_wrapper, config::region_offset_for(byte_size));
    offset_wrapper      = IRB.CreateBitCast(offset_wrapper, type->getPointerTo());

    auto allocation_id = IRB.CreateStructGEP(offset_wrapper, 0);
    IRB.CreateStore(typeIdConst, allocation_id);
    auto count = IRB.CreateStructGEP(offset_wrapper, 1);
    IRB.CreateStore(numElementsVal, count);
    auto user_data = IRB.CreateStructGEP(offset_wrapper, 2);
    alloca->replaceAllUsesWith(user_data);

    alloca->eraseFromParent();
  }

  return stack.size();
}

size_t InstrumentationStrategy::instrumentGlobal(const GlobalArgList& globals) {
  return tracker_instrumentation.instrumentGlobal(globals);
}

}  // namespace typeart::instrumentation::allocator