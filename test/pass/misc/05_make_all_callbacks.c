// RUN: %c-to-llvm %s | %apply-typeart -typeart-stack -S 2>&1 | %filecheck %s

#include "../../../lib/runtime/tracker/CallbackInterface.h"

int main(void) {
  int count     = 0;
  int type_id   = 10;
  size_t extent = 0;
  void* addr    = NULL;
  typeart_tracker_alloc(addr, type_id, extent);
  typeart_tracker_alloc_global(addr, type_id, extent);
  typeart_tracker_alloc_stack(addr, type_id, extent);
  typeart_tracker_free(addr);
  typeart_tracker_leave_scope(count);

  // called (only) from OpenMP context:
  typeart_tracker_alloc_omp(addr, type_id, extent);
  typeart_tracker_alloc_stack_omp(addr, type_id, extent);
  typeart_tracker_free_omp(addr);
  typeart_tracker_leave_scope_omp(count);
  return 0;
}

// CHECK:      TypeArtPass [Heap & Stack]