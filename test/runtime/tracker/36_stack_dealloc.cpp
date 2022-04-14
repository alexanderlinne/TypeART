// clang-format off
// RUN: %run %s --manual --compile_flags "-std=c++17" 2>&1 | %filecheck %s
// REQUIRES: tracker
// clang-format on

// TODO: how to handle this test with alloc_ids?
// XFAIL: *

#include "runtime/tracker/CallbackInterface.h"
#include "util.h"

#include <stdio.h>

int main(int argc, char** argv) {
  const int type{6};
  const size_t extent{6};
  double d[extent];

  // CHECK: [Error]{{.*}}Stack is smaller than requested de-allocation count. alloca_count: 1. size: 0
  typeart_tracker_leave_scope(1);
  // CHECK: [Error]{{.*}}Stack is smaller than requested de-allocation count. alloca_count: 12. size: 0
  typeart_tracker_leave_scope(12);

  // CHECK: [Trace] Alloc 0x{{[0-9a-f]+}} {{[0-9]*}} 6 double 8 6
  typeart_tracker_alloc_stack(reinterpret_cast<const void*>(&d[0]), type, extent);

  // CHECK: [Trace] Freeing stack (1)  1
  // CHECK: [Trace] Free 0x{{[0-9a-f]+}} {{[0-9]*}} 6 double 8 6
  // CHECK: [Trace] Stack after free: 0
  typeart_tracker_leave_scope(1);

  // CHECK: [Error]{{.*}}Stack is smaller than requested de-allocation count. alloca_count: 1. size: 0
  typeart_tracker_leave_scope(1);

  // CHECK: [Trace] Alloc 0x{{[0-9a-f]+}} {{[0-9]*}} 6 double 8 1
  typeart_tracker_alloc_stack(reinterpret_cast<const void*>(&d[0]), type, 1);
  // CHECK: [Trace] Alloc 0x{{[0-9a-f]+}} {{[0-9]*}} 6 double 8 1
  typeart_tracker_alloc_stack(reinterpret_cast<const void*>(&d[1]), type, 1);
  // CHECK: [Error]{{.*}}Stack is smaller than requested de-allocation count. alloca_count: 3. size: 2
  // CHECK: [Trace] Freeing stack (2)  2
  typeart_tracker_leave_scope(3);
  return 0;
}
