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

#include "analysis/MemOpData.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/iterator.h"
#include "llvm/Support/Casting.h"

#include <cstddef>
#include <memory>

namespace llvm {
class Value;
}  // namespace llvm

namespace typeart::instrumentation {

struct ArgStrId {
  static constexpr char pointer[]       = "pointer";
  static constexpr char type_id[]       = "type_id";
  static constexpr char type_size[]     = "type_size";
  static constexpr char byte_count[]    = "byte_count";
  static constexpr char element_count[] = "element_count";
  static constexpr char realloc_ptr[]   = "realloc_ptr";
  static constexpr char meta_id[]       = "meta_id";
};

struct ArgMap {
  using ID            = ArgStrId;
  using ArgsContainer = llvm::StringMap<llvm::Value*>;
  using Key           = ArgsContainer::key_type;
  ArgsContainer args;

  ArgsContainer::mapped_type& operator[](Key key) {
    return args[key];
  }

  auto lookup(Key key) const -> llvm::Value* {
    return args.lookup(key);
  }

  template <typename T>
  T* get_as(Key key) const {
    T* elem{nullptr};
    if (auto it = args.find(key); it != args.end()) {
      elem = llvm::dyn_cast<T>(it->second);
    }
    return elem;
  }

  llvm::Value* get_value(Key key) const {
    return get_as<llvm::Value>(key);
  }
};

namespace detail {
template <typename Data>
struct MemContainer {
  Data mem_data;
  ArgMap args;
};
}  // namespace detail

using HeapContainer   = detail::MemContainer<MallocData>;
using FreeContainer   = detail::MemContainer<FreeData>;
using StackContainer  = detail::MemContainer<AllocaData>;
using GlobalContainer = detail::MemContainer<GlobalData>;

using HeapArgList   = llvm::SmallVector<HeapContainer, 16>;
using FreeArgList   = llvm::SmallVector<FreeContainer, 16>;
using StackArgList  = llvm::SmallVector<StackContainer, 16>;
using GlobalArgList = llvm::SmallVector<GlobalContainer, 8>;

class ArgumentParser {
 public:
  virtual HeapArgList collectHeap(const MallocDataList& mallocs)     = 0;
  virtual FreeArgList collectFree(const FreeDataList& frees)         = 0;
  virtual StackArgList collectStack(const AllocaDataList& frees)     = 0;
  virtual GlobalArgList collectGlobal(const GlobalDataList& globals) = 0;
  virtual ~ArgumentParser()                                          = default;
};

class InstrumentationStrategy {
 public:
  virtual size_t instrumentHeap(const HeapArgList& heap)        = 0;
  virtual size_t instrumentFree(const FreeArgList& frees)       = 0;
  virtual size_t instrumentStack(const StackArgList& frees)     = 0;
  virtual size_t instrumentGlobal(const GlobalArgList& globals) = 0;
  virtual ~InstrumentationStrategy()                            = default;
};

class TypeArtInstrumentation {
 private:
  std::unique_ptr<ArgumentParser> parser;
  std::unique_ptr<InstrumentationStrategy> strategy;

 public:
  TypeArtInstrumentation(std::unique_ptr<ArgumentParser> parser, std::unique_ptr<InstrumentationStrategy> strategy);

  size_t handleHeap(const MallocDataList& mallocs);
  size_t handleFree(const FreeDataList& frees);
  size_t handleStack(const AllocaDataList& frees);
  size_t handleGlobal(const GlobalDataList& globals);
};

}  // namespace typeart::instrumentation
