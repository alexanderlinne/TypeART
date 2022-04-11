#include "CallbackInterface.h"

#include "runtime/Runtime.h"

#define TYPEART_RUNTIME_GUARD     \
  typeart::RTGuard GUARDNAME;     \
  if (!GUARDNAME.shouldTrack()) { \
    return;                       \
  }

void typeart_tracker_alloc(const void* addr, int typeId, size_t count) {
  TYPEART_RUNTIME_GUARD;
  const void* retAddr = __builtin_return_address(0);
  typeart::RuntimeSystem::get().allocTracker.onAlloc(addr, typeId, count, retAddr);
}

void typeart_tracker_alloc_stack(const void* addr, int typeId, size_t count) {
  TYPEART_RUNTIME_GUARD;
  const void* retAddr = __builtin_return_address(0);
  typeart::RuntimeSystem::get().allocTracker.onAllocStack(addr, typeId, count, retAddr);
}

void typeart_tracker_alloc_global(const void* addr, int typeId, size_t count) {
  TYPEART_RUNTIME_GUARD;
  const void* retAddr = __builtin_return_address(0);
  typeart::RuntimeSystem::get().allocTracker.onAllocGlobal(addr, typeId, count, retAddr);
}

void typeart_tracker_free(const void* addr) {
  TYPEART_RUNTIME_GUARD;
  const void* retAddr = __builtin_return_address(0);
  typeart::RuntimeSystem::get().allocTracker.onFreeHeap(addr, retAddr);
}

void typeart_tracker_leave_scope(int alloca_count) {
  TYPEART_RUNTIME_GUARD;
  const void* retAddr = __builtin_return_address(0);
  typeart::RuntimeSystem::get().allocTracker.onLeaveScope(alloca_count, retAddr);
}

void typeart_tracker_alloc_omp(const void* addr, int typeId, size_t count) {
  TYPEART_RUNTIME_GUARD;
  const void* retAddr = __builtin_return_address(0);
  typeart::RuntimeSystem::get().allocTracker.onAlloc(addr, typeId, count, retAddr);
  typeart::RuntimeSystem::get().recorder.incOmpContextHeap();
}

void typeart_tracker_alloc_stack_omp(const void* addr, int typeId, size_t count) {
  TYPEART_RUNTIME_GUARD;
  const void* retAddr = __builtin_return_address(0);
  typeart::RuntimeSystem::get().allocTracker.onAllocStack(addr, typeId, count, retAddr);
  typeart::RuntimeSystem::get().recorder.incOmpContextStack();
}

void typeart_tracker_free_omp(const void* addr) {
  TYPEART_RUNTIME_GUARD;
  const void* retAddr = __builtin_return_address(0);
  typeart::RuntimeSystem::get().allocTracker.onFreeHeap(addr, retAddr);
  typeart::RuntimeSystem::get().recorder.incOmpContextFree();
}

void typeart_tracker_leave_scope_omp(int alloca_count) {
  TYPEART_RUNTIME_GUARD;
  const void* retAddr = __builtin_return_address(0);
  typeart::RuntimeSystem::get().allocTracker.onLeaveScope(alloca_count, retAddr);
}
