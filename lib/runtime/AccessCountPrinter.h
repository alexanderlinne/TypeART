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

#ifndef TYPEART_ACCESSCOUNTPRINTER_H
#define TYPEART_ACCESSCOUNTPRINTER_H

#include "runtime/AccessCounter.hpp"
#include "runtime/tracker/Types.hpp"
#include "support/Table.h"

#include <fmt/core.h>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace typeart::softcounter {
namespace memory {
struct MemOverhead {
  static constexpr auto pointerMapSize = sizeof(tracker::RuntimeT::PointerMap);  // Map overhead
  static constexpr auto perNodeSizeMap =
      64U + sizeof(tracker::RuntimeT::MapEntry);  // rough estimate, not applicable to btree; 64U is internal node size
  static constexpr auto stackVectorSize  = sizeof(tracker::RuntimeT::Stack);       // Stack overhead
  static constexpr auto perNodeSizeStack = sizeof(tracker::RuntimeT::StackEntry);  // Stack allocs
  double stack{0};
  double map{0};
};
inline MemOverhead estimate(Counter stack_max, Counter heap_max, Counter global_max, const double scale = 1024.0) {
  MemOverhead mem;
  mem.stack = double(MemOverhead::stackVectorSize +
                     MemOverhead::perNodeSizeStack * std::max<size_t>(tracker::RuntimeT::StackReserve, stack_max)) /
              scale;
  mem.map =
      double(MemOverhead::pointerMapSize + MemOverhead::perNodeSizeMap * (stack_max + heap_max + global_max)) / scale;
  return mem;
}
}  // namespace memory

template <typename Recorder>
void serialize(const Recorder& r, std::ostringstream& buf) {
  if constexpr (std::is_same_v<Recorder, NoneRecorder>) {
    return;
  } else {
    const auto memory_use = memory::estimate(r.getMaxStackAllocs(), r.getMaxHeapAllocs(), r.getGlobalAllocs());

    Table t("Alloc Stats from softcounters");
    t.wrap_length = true;
    t.put(Row::make("Total heap", r.getHeapAllocs(), r.getHeapArray()));
    t.put(Row::make("Total stack", r.getStackAllocs(), r.getStackArray()));
    t.put(Row::make("Total global", r.getGlobalAllocs(), r.getGlobalArray()));
    t.put(Row::make("Max. Heap Allocs", r.getMaxHeapAllocs()));
    t.put(Row::make("Max. Stack Allocs", r.getMaxStackAllocs()));
    t.put(Row::make("Addresses checked", r.getAddrChecked()));
    t.put(Row::make("Distinct Addresses checked", r.getSeen().size()));
    t.put(Row::make("Addresses re-used", r.getAddrReuses()));
    t.put(Row::make("Addresses missed", r.getAddrMissing()));
    t.put(Row::make("Distinct Addresses missed", r.getMissing().size()));
    t.put(Row::make("Total free heap", r.getHeapAllocsFree(), r.getHeapArrayFree()));
    t.put(Row::make("Total free stack", r.getStackAllocsFree(), r.getStackArrayFree()));
    t.put(Row::make("OMP Stack/Heap/Free", r.getOmpStackCalls(), r.getOmpHeapCalls(), r.getOmpFreeCalls()));
    t.put(Row::make("Null/Zero/NullZero Addr", r.getNullAlloc(), r.getZeroAlloc(), r.getNullAndZeroAlloc()));
    t.put(Row::make("Estimated memory use (KiB)", size_t(std::round(memory_use.map + memory_use.stack))));
    t.put(Row::make("Bytes per node map/stack", memory::MemOverhead::perNodeSizeMap,
                    memory::MemOverhead::perNodeSizeStack));

    t.print(buf);

    std::set<const meta::di::Type*> type_set;
    const auto fill_set = [&type_set](const auto& map) {
      for (const auto& [key, val] : map) {
        type_set.insert(key);
      }
    };
    fill_set(r.getHeapAlloc());
    fill_set(r.getGlobalAlloc());
    fill_set(r.getStackAlloc());
    fill_set(r.getHeapFree());
    fill_set(r.getStackFree());

    const auto count = [](const auto& map, auto id) {
      auto it = map.find(id);
      if (it != map.end()) {
        return it->second;
      }
      return 0LL;
    };

    Table type_table("Allocation type detail (heap, stack, global)");
    type_table.table_header = '#';
    for (auto type : type_set) {
      if (type == nullptr) {
        continue;
      }
      type_table.put(Row::make(type->get_pretty_name(), count(r.getHeapAlloc(), type), count(r.getStackAlloc(), type),
                               count(r.getGlobalAlloc(), type)));
    }

    type_table.print(buf);

    Table type_table_free("Free allocation type detail (heap, stack)");
    type_table_free.table_header = '#';
    for (auto type : type_set) {
      if (type == nullptr) {
        continue;
      }
      type_table_free.put(
          Row::make(type->get_pretty_name(), count(r.getHeapFree(), type), count(r.getStackFree(), type)));
    }

    type_table_free.print(buf);

    const auto numThreads = r.getNumThreads();
    std::stringstream ss;
    ss << "Per-thread counter values (" << numThreads << " threads)";
    Table thread_table(ss.str());
    thread_table.table_header = '#';

    auto print_thread_row = [&thread_table](std::string name, const std::vector<Counter>& vals) {
      Row row(std::move(name));
      for (const auto& val : vals) {
        row.put(Cell(val));
      }
      thread_table.put(std::move(row));
    };

    print_thread_row("Thread Heap Allocs", r.getHeapAllocsThreadData());
    print_thread_row("Thread Heap Arrays", r.getHeapArrayThreadData());
    print_thread_row("Thread Heap Allocs Free", r.getHeapAllocsFreeThreadData());
    print_thread_row("Thread Heap Arrays Free", r.getHeapArrayFreeThreadData());
    print_thread_row("Thread Stack Allocs", r.getStackAllocsThreadData());
    print_thread_row("Thread Stack Arrays", r.getStackArrayThreadData());
    print_thread_row("Thread Max. Stack Allocs", r.getMaxStackAllocsThreadData());
    print_thread_row("Thread Stack Allocs Free", r.getStackAllocsFreeThreadData());
    print_thread_row("Thread Stack Array Free", r.getStackArrayFreeThreadData());
    thread_table.print(buf);
  }
}
}  // namespace typeart::softcounter

#endif  // TYPEART_ACCESSCOUNTPRINTER_H
