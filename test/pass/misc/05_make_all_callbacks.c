// RUN: %c-to-llvm -I../../../lib -I../../../include/typeart %s | %apply-typeart -typeart-stack -S > %s.log 2>&1
// RUN: cat %s.log | %filecheck %s

#include "runtime/tracker/CallbackInterface.h"

int main(void) {
  int count               = 0;
  alloc_id_value alloc_id = 1;
  size_t extent           = 0;
  void* addr              = NULL;
  typeart_tracker_alloc(addr, alloc_id, extent);
  typeart_tracker_alloc_global(addr, alloc_id, extent);
  typeart_tracker_alloc_stack(addr, alloc_id, extent);
  typeart_tracker_free(addr);
  typeart_tracker_leave_scope(count);

  // called (only) from OpenMP context:
  typeart_tracker_alloc_omp(addr, alloc_id, extent);
  typeart_tracker_alloc_stack_omp(addr, alloc_id, extent);
  typeart_tracker_free_omp(addr);
  typeart_tracker_leave_scope_omp(count);
  return 0;
}

// CHECK:      TypeArtPass [Heap & Stack]