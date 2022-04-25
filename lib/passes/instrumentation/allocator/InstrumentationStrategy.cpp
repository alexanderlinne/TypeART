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
#include "db/Database.hpp"
#include "runtime/allocator/Config.h"
#include "support/Logger.hpp"
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

InstrumentationStrategy::InstrumentationStrategy(cl::InstrumentationMode mode, llvm::Module& m)
    : instrumentation::InstrumentationStrategy(),
      mode(mode),
      type_art_functions(m),
      instr_helper(m),
      tracker_instrumentation(m, false) {
}

size_t InstrumentationStrategy::instrumentHeap(const HeapArgList& heap) {
  for (const auto& [malloc, args] : heap) {
    auto malloc_call = llvm::dyn_cast<llvm::CallBase>(args.get_as<llvm::Instruction>(ArgMap::ID::pointer));
    llvm::IRBuilder<> builder(malloc_call);
    auto alloc_id  = args.get_value(ArgMap::ID::alloc_id);
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

    auto replace_call_base = [&builder](llvm::CallBase* original, llvm::Function* replacement,
                                        llvm::ArrayRef<llvm::Value*> args) {
      auto replacement_call = (llvm::Value*)nullptr;
      if (auto invoke_inst = llvm::dyn_cast<llvm::InvokeInst>(original)) {
        const auto normal = invoke_inst->getNormalDest();
        const auto unwind = invoke_inst->getUnwindDest();
        replacement_call  = builder.CreateInvoke(replacement, normal, unwind, args);
      } else if (auto call_inst = llvm::dyn_cast<llvm::CallInst>(original)) {
        replacement_call = builder.CreateCall(replacement, args);
      } else if (auto call_br_inst = llvm::dyn_cast<llvm::CallBrInst>(original)) {
        const auto def      = call_br_inst->getDefaultDest();
        const auto indirect = call_br_inst->getIndirectDests();
        replacement_call    = builder.CreateCallBr(replacement, def, indirect, args);
      } else {
        llvm::errs() << "Unexpected CallBase subtype:\n";
        original->print(llvm::errs());
        llvm::errs() << "\n";
        abort();
      }
      original->replaceAllUsesWith(replacement_call);
      original->eraseFromParent();
    };

    static std::map<std::string, llvm::Function*> function_map = {
        {"malloc", type_art_functions.allocator_malloc}, {"realloc", type_art_functions.allocator_realloc},
        {"calloc", type_art_functions.allocator_calloc}, {"_Znwm", type_art_functions.allocator__Znwm},
        {"_Znam", type_art_functions.allocator__Znam},
    };

    auto function_name = malloc_call->getCalledFunction()->getName();
    if (function_name == "malloc" || function_name == "_Znwm" || function_name == "_Znam") {
      replace_call_base(malloc_call, function_map[function_name],
                        {alloc_id, element_count, malloc_call->getArgOperand(0)});
    } else if (function_name == "calloc" || function_name == "realloc") {
      replace_call_base(malloc_call, function_map[function_name],
                        {alloc_id, element_count, malloc_call->getArgOperand(0), malloc_call->getArgOperand(1)});
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
  return {};
}

llvm::AllocaInst* InstrumentationStrategy::createWrapperAlloca(llvm::AllocaInst* alloca, bool is_vla) {
  namespace config          = typeart::runtime::allocator::config;
  const auto alloc_id_type  = instr_helper.getTypeFor(IType::alloc_id);
  const auto count_type     = instr_helper.getTypeFor(IType::extent);
  auto& ctx                 = alloca->getContext();
  const auto& dl            = alloca->getModule()->getDataLayout();
  const auto allocated_type = alloca->getAllocatedType();
  const auto required_size =
      dl.getTypeAllocSize(allocated_type).getFixedSize() + config::stack::metadata_byte_size(is_vla);
  const auto allocation_size = config::stack::allocation_size_for(required_size);
  auto name         = fmt::format("Typeart_Wrapper_{}_Align_{}{}", typeNameFor(allocated_type), alloca->getAlignment(),
                          is_vla ? "_VLA" : "");
  auto wrapper_type = alloca->getModule()->getTypeByName(name);
  if (wrapper_type == nullptr) {
    const auto alloca_padding_type = llvm::ArrayType::get(llvm::Type::getInt8Ty(ctx), allocation_size - required_size);
    if (is_vla) {
      const auto count_padding_type = llvm::ArrayType::get(llvm::Type::getInt8Ty(ctx), config::count_padding);
      wrapper_type                  = llvm::StructType::create(
                           {allocated_type, alloca_padding_type, count_type, count_padding_type, alloc_id_type}, name,
                           /* packed = */ true);
    } else {
      wrapper_type = llvm::StructType::create({allocated_type, alloca_padding_type, alloc_id_type}, name,
                                              /* packed = */ true);
    }
  }
  auto required_alignment = config::stack::alignment_for(allocation_size);
  assert(required_alignment >= alloca->getAlignment());
  llvm::IRBuilder<> IRB(alloca->getNextNode());
  auto wrapper_alloca = IRB.CreateAlloca(wrapper_type);
  wrapper_alloca->setAlignment(llvm::MaybeAlign{required_alignment});
  return wrapper_alloca;
}

size_t InstrumentationStrategy::instrumentStack(const StackArgList& stack) {
  namespace config = typeart::runtime::allocator::config;
  for (const auto& [sdata, args] : stack) {
    auto* alloca   = args.get_as<llvm::AllocaInst>(ArgMap::ID::pointer);
    auto& ctx      = alloca->getContext();
    const auto& dl = alloca->getModule()->getDataLayout();
    if (mode == cl::InstrumentationMode::before_optimization || mode == cl::InstrumentationMode::combined) {
      llvm::IRBuilder<> IRB(alloca->getNextNode());
      auto allocIdConst         = args.get_value(ArgMap::ID::alloc_id);
      auto elementCountConst    = args.get_value(ArgMap::ID::element_count);
      const auto wrapper_alloca = createWrapperAlloca(alloca, sdata.is_vla);

      const auto alloc_id = IRB.CreateStructGEP(wrapper_alloca, sdata.is_vla ? 4 : 2);
      IRB.CreateStore(allocIdConst, alloc_id);

      if (sdata.is_vla) {
        const auto element_count = IRB.CreateStructGEP(wrapper_alloca, 2);
        IRB.CreateStore(elementCountConst, element_count);
      }

      const auto user_data = IRB.CreateStructGEP(wrapper_alloca, 0);
      alloca->replaceAllUsesWith(user_data);
      if (alloca->isTerminator()) {
        abort();
      }
      alloca->eraseFromParent();
      alloca = wrapper_alloca;
    }
    if (mode == cl::InstrumentationMode::after_optimization || mode == cl::InstrumentationMode::combined) {
      const auto allocated_type = alloca->getAllocatedType();
      if (allocated_type->getStructName().startswith("Typeart_Wrapper_")) {
        auto byte_size = dl.getTypeAllocSize(allocated_type).getFixedSize();
        llvm::IRBuilder<> IRB(alloca->getNextNode());
        const auto casted_wrapper = (llvm::Instruction*)IRB.CreateBitCast(alloca, llvm::Type::getInt8PtrTy(ctx));
        const auto offset_casted  = IRB.CreateGEP(
             casted_wrapper,
             llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx), config::stack::region_offset_for(byte_size)));
        auto offset_wrapper = IRB.CreateBitCast(offset_casted, allocated_type->getPointerTo());
        alloca->replaceAllUsesWith(offset_wrapper);
        casted_wrapper->setOperand(0, alloca);
      }
    }
  }

  return stack.size();
}

size_t InstrumentationStrategy::instrumentGlobal(const GlobalArgList& globals) {
  return tracker_instrumentation.instrumentGlobal(globals);
}

}  // namespace typeart::instrumentation::allocator