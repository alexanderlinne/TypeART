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
  auto result = tracker_parser.collectHeap(mallocs);
  for (auto& data : result) {
    const auto type_id = llvm::dyn_cast<llvm::ConstantInt>(data.args[ArgMap::ID::type_id])->getZExtValue();
    const auto allocId =
        type_m->getOrRegisterAllocation(static_cast<type_id_t::value_type>(type_id), {}, {});
    data.args[ArgMap::ID::alloc_id] = instr_helper.getConstantFor(IType::alloc_id, allocId.value());
  }
  return result;
}

FreeArgList ArgumentParser::collectFree(const FreeDataList& frees) {
  return {};
}

StackArgList ArgumentParser::collectStack(const AllocaDataList& allocs) {
  namespace config = typeart::runtime::allocator::config;
  auto result      = tracker_parser.collectStack(allocs);
  for (auto& data : result) {
    const auto& alloca     = data.mem_data.alloca;
    const auto elementType = alloca->getAllocatedType();
    auto elementCount      = std::optional<size_t>{};
    if (!data.mem_data.is_vla) {
      size_t arraySize = data.mem_data.array_size;
      if (elementType->isArrayTy()) {
        arraySize = arraySize * typeart::util::type::getArrayLengthFlattened(elementType);
      }
      elementCount = {arraySize};
    }
    auto type_id         = llvm::dyn_cast<llvm::ConstantInt>(data.args[ArgMap::ID::type_id])->getZExtValue();
    auto base_ptr_offset = config::stack::base_ptr_offset_for(alloca->getAlignment(), data.mem_data.is_vla);
    auto alloc_id        = type_m->getOrRegisterAllocation(static_cast<type_id_t::value_type>(type_id),
                                                           elementCount, base_ptr_offset);
    data.args[ArgMap::ID::alloc_id] = instr_helper.getConstantFor(IType::alloc_id, alloc_id.value());
  }
  return result;
}

GlobalArgList ArgumentParser::collectGlobal(const GlobalDataList& globals) {
  return tracker_parser.collectGlobal(globals);
}

}  // namespace typeart::instrumentation::allocator
