#include "CallbackInterface.h"

#include "runtime/Runtime.hpp"

#define TYPEART_RUNTIME_GUARD             \
  auto GUARDNAME = Runtime::scopeGuard(); \
  if (!GUARDNAME.shouldTrack()) {         \
    return;                               \
  }

using namespace typeart;
using namespace typeart::runtime;

extern "C" {

void typeart_tracker_alloc(const void* addr, alloc_id_t::value_type alloc_id, size_t count) {
  TYPEART_RUNTIME_GUARD;
  const void* retAddr = __builtin_return_address(0);
  Runtime::getTracker().onAlloc(addr, alloc_id, count, retAddr);
}

void typeart_tracker_alloc_stack(const void* addr, alloc_id_t::value_type alloc_id, size_t count) {
  TYPEART_RUNTIME_GUARD;
  const void* retAddr = __builtin_return_address(0);
  Runtime::getTracker().onAllocStack(addr, alloc_id, count, retAddr);
}

void typeart_tracker_alloc_global(const void* addr, alloc_id_t::value_type alloc_id, size_t count) {
  TYPEART_RUNTIME_GUARD;
  const void* retAddr = __builtin_return_address(0);
  Runtime::getTracker().onAllocGlobal(addr, alloc_id, count, retAddr);
}

void typeart_tracker_free(const void* addr) {
  TYPEART_RUNTIME_GUARD;
  const void* retAddr = __builtin_return_address(0);
  Runtime::getTracker().onFreeHeap(addr, retAddr);
}

void typeart_tracker_leave_scope(int alloca_count) {
  TYPEART_RUNTIME_GUARD;
  const void* retAddr = __builtin_return_address(0);
  Runtime::getTracker().onLeaveScope(alloca_count, retAddr);
}

void typeart_tracker_alloc_omp(const void* addr, alloc_id_t::value_type alloc_id, size_t count) {
  TYPEART_RUNTIME_GUARD;
  const void* retAddr = __builtin_return_address(0);
  Runtime::getTracker().onAlloc(addr, alloc_id, count, retAddr);
  Runtime::getRecorder().incOmpContextHeap();
}

void typeart_tracker_alloc_stack_omp(const void* addr, alloc_id_t::value_type alloc_id, size_t count) {
  TYPEART_RUNTIME_GUARD;
  const void* retAddr = __builtin_return_address(0);
  Runtime::getTracker().onAllocStack(addr, alloc_id, count, retAddr);
  Runtime::getRecorder().incOmpContextStack();
}

void typeart_tracker_free_omp(const void* addr) {
  TYPEART_RUNTIME_GUARD;
  const void* retAddr = __builtin_return_address(0);
  Runtime::getTracker().onFreeHeap(addr, retAddr);
  Runtime::getRecorder().incOmpContextFree();
}

void typeart_tracker_leave_scope_omp(int alloca_count) {
  TYPEART_RUNTIME_GUARD;
  const void* retAddr = __builtin_return_address(0);
  Runtime::getTracker().onLeaveScope(alloca_count, retAddr);
}
}
