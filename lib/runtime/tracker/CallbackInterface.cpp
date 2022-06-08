#include "CallbackInterface.h"

#include "runtime/Runtime.hpp"
#include "runtime/tracker/Tracker.hpp"

#define TYPEART_RUNTIME_GUARD     \
  auto GUARDNAME = ScopeGuard{};  \
  if (!GUARDNAME.shouldTrack()) { \
    return;                       \
  }

using namespace typeart;

extern "C" {

void typeart_tracker_alloc(const void* addr, alloc_id_t::value_type alloc_id, size_t count) {
  TYPEART_RUNTIME_GUARD;
  const void* retAddr = __builtin_return_address(0);
  tracker::Tracker::get().onAlloc(addr, alloc_id_t{alloc_id}, count, retAddr);
}

void typeart_tracker_alloc_stack(const void* addr, alloc_id_t::value_type alloc_id, size_t count) {
  TYPEART_RUNTIME_GUARD;
  const void* retAddr = __builtin_return_address(0);
  tracker::Tracker::get().onAllocStack(addr, alloc_id, count, retAddr);
}

void typeart_tracker_alloc_global(const void* addr, alloc_id_t::value_type alloc_id, size_t count) {
  TYPEART_RUNTIME_GUARD;
  const void* retAddr = __builtin_return_address(0);
  tracker::Tracker::get().onAllocGlobal(addr, alloc_id, count, retAddr);
}

void typeart_tracker_free(const void* addr) {
  TYPEART_RUNTIME_GUARD;
  const void* retAddr = __builtin_return_address(0);
  tracker::Tracker::get().onFreeHeap(addr, retAddr);
}

void typeart_tracker_leave_scope(int alloca_count) {
  TYPEART_RUNTIME_GUARD;
  const void* retAddr = __builtin_return_address(0);
  tracker::Tracker::get().onLeaveScope(alloca_count, retAddr);
}

void typeart_tracker_alloc_omp(const void* addr, alloc_id_t::value_type alloc_id, size_t count) {
  TYPEART_RUNTIME_GUARD;
  const void* retAddr = __builtin_return_address(0);
  tracker::Tracker::get().onAlloc(addr, alloc_id_t{alloc_id}, count, retAddr);
  getRecorder().incOmpContextHeap();
}

void typeart_tracker_alloc_stack_omp(const void* addr, alloc_id_t::value_type alloc_id, size_t count) {
  TYPEART_RUNTIME_GUARD;
  const void* retAddr = __builtin_return_address(0);
  tracker::Tracker::get().onAllocStack(addr, alloc_id, count, retAddr);
  getRecorder().incOmpContextStack();
}

void typeart_tracker_free_omp(const void* addr) {
  TYPEART_RUNTIME_GUARD;
  const void* retAddr = __builtin_return_address(0);
  tracker::Tracker::get().onFreeHeap(addr, retAddr);
  getRecorder().incOmpContextFree();
}

void typeart_tracker_leave_scope_omp(int alloca_count) {
  TYPEART_RUNTIME_GUARD;
  const void* retAddr = __builtin_return_address(0);
  tracker::Tracker::get().onLeaveScope(alloca_count, retAddr);
}
}
