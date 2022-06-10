// clang-format off
// RUN: %run %s 2>&1 | %filecheck %s
// REQUIRES: tracker
// clang-format on

#include "util.hpp"

#include <stdio.h>

int main(int argc, char** argv) {
  const auto meta_id = create_fake_double_stack_allocation();

  // CHECK: [Trace] Alloc stack 0x{{[0-9a-f]+}} of type [1 x double[6]]
  double d[6];

  // CHECK: [Trace] Free stack 0x{{[0-9a-f]+}} of type [1 x int]
  // CHECK: [Trace] Free stack 0x{{[0-9a-f]+}} of type [1 x const typeart::meta_id_t]
  // CHECK: [Trace] Free stack 0x{{[0-9a-f]+}} of type [1 x double[6]]
  typeart_tracker_leave_scope(3);
  // CHECK: [Error]{{.*}}Stack is smaller than requested de-allocation count. alloca_count: 12. size: 0
  typeart_tracker_leave_scope(12);

  // CHECK: [Trace] Alloc stack 0x{{[0-9a-f]+}} of type [6 x double]
  typeart_tracker_alloc_stack(reinterpret_cast<const void*>(&d[0]), meta_id.value(), 6);

  // CHECK: [Trace] Freeing 1 stack entries
  // CHECK: [Trace] Free stack 0x{{[0-9a-f]+}} of type [6 x double]
  // CHECK: [Trace] 0 remaining stack entries
  typeart_tracker_leave_scope(1);

  // CHECK: [Error]{{.*}}Stack is smaller than requested de-allocation count. alloca_count: 1. size: 0
  typeart_tracker_leave_scope(1);

  // CHECK: [Trace] Alloc stack 0x{{[0-9a-f]+}} of type [1 x double]
  typeart_tracker_alloc_stack(reinterpret_cast<const void*>(&d[0]), meta_id.value(), 1);
  // CHECK: [Trace] Alloc stack 0x{{[0-9a-f]+}} of type [1 x double]
  typeart_tracker_alloc_stack(reinterpret_cast<const void*>(&d[1]), meta_id.value(), 1);
  // CHECK: [Error]{{.*}}Stack is smaller than requested de-allocation count. alloca_count: 3. size: 2
  // CHECK: [Trace] Freeing 2 stack entries
  typeart_tracker_leave_scope(3);
  return 0;
}
