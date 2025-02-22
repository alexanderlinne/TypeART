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

#include "meta/Meta.hpp"
#include "support/Logger.hpp"
#include "support/TypeUtil.h"
#include "support/Util.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/raw_ostream.h"

#include <cassert>
#include <cstddef>

namespace llvm {
class DataLayout;
class Value;
}  // namespace llvm

namespace tu = typeart::util::type;
using namespace llvm;

namespace typeart::instrumentation::tracker {

ArgumentParser::ArgumentParser(llvm::Module& m, meta::LLVMMetadataConverter& converter)
    : instrumentation::ArgumentParser(), module(&m), instr_helper(m), converter(&converter) {
}

HeapArgList ArgumentParser::collectHeap(const MallocDataList& mallocs) {
  HeapArgList list;
  list.reserve(mallocs.size());
  const llvm::DataLayout& dl = module->getDataLayout();
  for (const MallocData& mdata : mallocs) {
    ArgMap arg_map;
    const auto malloc_call      = mdata.call;
    Value* pointer              = malloc_call;
    BitCastInst* primaryBitcast = mdata.primary;
    auto kind                   = mdata.kind;

    // Number of bytes allocated
    auto mallocArg = malloc_call->getOperand(0);

    // Number of bytes per element, 1 for void*
    unsigned typeSize = tu::getTypeSizeInBytes(malloc_call->getType()->getPointerElementType(), dl);

    // Use the first cast as the determining type (if there is any)
    if (primaryBitcast != nullptr) {
      auto* dstPtrType = primaryBitcast->getDestTy()->getPointerElementType();

      typeSize = tu::getTypeSizeInBytes(dstPtrType, dl);

      // Resolve arrays
      // TODO: Write tests for this case
      if (dstPtrType->isArrayTy()) {
        dstPtrType = tu::getArrayElementType(dstPtrType);
      }

    } else {
      LOG_WARNING("Primary bitcast is null. malloc: {}", util::dump(*malloc_call));
    }

    Value* typeSizeConst = instr_helper.getConstantFor(IType::extent, typeSize);

    Value* elementCount{nullptr};
    Value* byte_count{nullptr};
    Value* realloc_ptr{nullptr};
    switch (kind) {
      case MemOpKind::NewLike:
        [[fallthrough]];
      case MemOpKind::MallocLike:
        if (mdata.array_cookie.hasValue()) {
          auto array_cookie_data = mdata.array_cookie.getValue();
          elementCount           = array_cookie_data.cookie_store->getValueOperand();
          pointer                = array_cookie_data.array_ptr_gep;
        }

        byte_count = mallocArg;
        break;
      case MemOpKind::CallocLike: {
        if (mdata.primary == nullptr) {
          // we need the second arg when the calloc type is identified as void* to calculate total bytes allocated
          typeSizeConst = malloc_call->getOperand(1);
        }
        elementCount = malloc_call->getOperand(0);
        break;
      }
      case MemOpKind::ReallocLike:
        realloc_ptr = malloc_call->getOperand(0);
        byte_count  = malloc_call->getOperand(1);
        break;
      case MemOpKind::AlignedAllocLike:
        byte_count = malloc_call->getArgOperand(1);
        break;
      default:
        LOG_ERROR("Unknown malloc kind. Not instrumenting. {}", util::dump(*malloc_call));
        // TODO see above continues
        continue;
    }

    assert(mdata.type != nullptr && mdata.location != nullptr);
    const auto meta        = converter->createHeapAllocation(*mdata.type, *mdata.location);
    const auto metaIdConst = instr_helper.getConstantFor(IType::alloc_id, meta->get_id().value());

    arg_map[ArgMap::ID::pointer]       = pointer;
    arg_map[ArgMap::ID::type_size]     = typeSizeConst;
    arg_map[ArgMap::ID::byte_count]    = byte_count;
    arg_map[ArgMap::ID::element_count] = elementCount;
    arg_map[ArgMap::ID::realloc_ptr]   = realloc_ptr;
    arg_map[ArgMap::ID::meta_id]       = metaIdConst;
    list.emplace_back(HeapArgList::value_type{mdata, arg_map});
  }

  return list;
}

FreeArgList ArgumentParser::collectFree(const FreeDataList& frees) {
  FreeArgList list;
  list.reserve(frees.size());
  for (const FreeData& fdata : frees) {
    ArgMap arg_map;
    auto free_call = fdata.call;

    Value* freeArg{nullptr};
    switch (fdata.kind) {
      case MemOpKind::DeleteLike:
        [[fallthrough]];
      case MemOpKind::FreeLike:
        freeArg = fdata.array_cookie_gep.hasValue() ? fdata.array_cookie_gep.getValue()->getPointerOperand()
                                                    : free_call->getOperand(0);
        break;
      default:
        LOG_ERROR("Unknown free kind. Not instrumenting. {}", util::dump(*free_call));
        continue;
    }

    arg_map[ArgMap::ID::pointer] = freeArg;
    list.emplace_back(FreeArgList::value_type{fdata, arg_map});
  }

  return list;
}

StackArgList ArgumentParser::collectStack(const AllocaDataList& allocs) {
  using namespace llvm;
  StackArgList list;
  list.reserve(allocs.size());

  std::optional<size_t> constantArraySize = {};
  for (const AllocaData& adata : allocs) {
    ArgMap arg_map;
    auto alloca           = adata.alloca;
    Value* numElementsVal = nullptr;
    // The length can be specified statically through the array type or as a separate argument.
    // Both cases are handled here.
    if (adata.is_vla) {
      numElementsVal = alloca->getArraySize();
      // This should not happen in generated IR code
      assert(!alloca->getAllocatedType()->isArrayTy() && "VLAs of array types are currently not supported.");
    } else {
      size_t arraySize  = adata.array_size;
      constantArraySize = arraySize;
      numElementsVal    = instr_helper.getConstantFor(IType::extent, arraySize);
    }

    assert(adata.alloca != nullptr);
    if (adata.local_variable == nullptr) {
      LOG_WARNING("Unknown stack type for {} in function {}. Skipped.", util::dump(*adata.alloca),
                  alloca->getFunction()->getName().str());
      continue;
    }

    assert(adata.location != nullptr);
    const auto meta = converter->createStackAllocation(*adata.local_variable, *adata.location, constantArraySize);
    const auto metaIdConst = instr_helper.getConstantFor(IType::alloc_id, meta->get_id().value());

    arg_map[ArgMap::ID::pointer]       = alloca;
    arg_map[ArgMap::ID::element_count] = numElementsVal;
    arg_map[ArgMap::ID::meta_id]       = metaIdConst;

    list.emplace_back(StackArgList::value_type{adata, arg_map});
  }

  return list;
}

GlobalArgList ArgumentParser::collectGlobal(const GlobalDataList& globals) {
  GlobalArgList list;
  list.reserve(globals.size());

  for (const GlobalData& gdata : globals) {
    ArgMap arg_map;
    auto global = gdata.global;

    if (gdata.global_variable == nullptr) {
      LOG_WARNING("Unknown global type for {}. Not instrumenting. ", util::dump(*gdata.global));
      continue;
    }

    auto* numElementsConst = instr_helper.getConstantFor(IType::extent, 1);
    const auto meta        = converter->createGlobalAllocation(*gdata.global_variable);
    const auto metaIdConst = instr_helper.getConstantFor(IType::alloc_id, meta->get_id().value());

    arg_map[ArgMap::ID::pointer]       = global;
    arg_map[ArgMap::ID::element_count] = numElementsConst;
    arg_map[ArgMap::ID::meta_id]       = metaIdConst;

    list.emplace_back(GlobalArgList::value_type{gdata, arg_map});
  }

  return list;
}

}  // namespace typeart::instrumentation::tracker
