#include "CallbackInterface.h"

#include "runtime/Runtime.h"

#define TYPEART_RUNTIME_GUARD             \
  auto GUARDNAME = Runtime::scopeGuard(); \
  if (!GUARDNAME.shouldTrack()) {         \
    return;                               \
  }

using namespace typeart::runtime;

extern "C" {

void typeart_tracker_alloc(const void* addr, int typeId, size_t count) {
  TYPEART_RUNTIME_GUARD;
  const void* retAddr = __builtin_return_address(0);
  Runtime::getTracker().onAlloc(addr, typeId, count, retAddr);
}

void typeart_tracker_alloc_stack(const void* addr, int typeId, size_t count) {
  TYPEART_RUNTIME_GUARD;
  const void* retAddr = __builtin_return_address(0);
  Runtime::getTracker().onAllocStack(addr, typeId, count, retAddr);
}

void typeart_tracker_alloc_global(const void* addr, int typeId, size_t count) {
  TYPEART_RUNTIME_GUARD;
  const void* retAddr = __builtin_return_address(0);
  Runtime::getTracker().onAllocGlobal(addr, typeId, count, retAddr);
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

void typeart_tracker_alloc_omp(const void* addr, int typeId, size_t count) {
  TYPEART_RUNTIME_GUARD;
  const void* retAddr = __builtin_return_address(0);
  Runtime::getTracker().onAlloc(addr, typeId, count, retAddr);
  Runtime::getRecorder().incOmpContextHeap();
}

void typeart_tracker_alloc_stack_omp(const void* addr, int typeId, size_t count) {
  TYPEART_RUNTIME_GUARD;
  const void* retAddr = __builtin_return_address(0);
  Runtime::getTracker().onAllocStack(addr, typeId, count, retAddr);
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
