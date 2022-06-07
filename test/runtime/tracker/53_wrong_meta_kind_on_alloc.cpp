// clang-format off
// RUN: %run %s 2>&1 | %filecheck %s
// REQUIRES: tracker

// clang-format on

#include "util.hpp"

using namespace typeart;

int main(int argc, char** argv) {
  auto heap_alloc_id  = create_fake_double_heap_alloc_id();
  auto stack_alloc_id = create_fake_double_stack_alloc_id();

  typeart_tracker_alloc(reinterpret_cast<const void*>(1UL), stack_alloc_id.value(), 1);
  typeart_tracker_free(reinterpret_cast<const void*>(1UL));

  typeart_tracker_alloc_stack(reinterpret_cast<const void*>(1UL), heap_alloc_id.value(), 1);
  typeart_tracker_leave_scope(1);

  // CHECK: [Error] Unexpected meta type. Expected HeapAllocation, but found StackAllocation
  // CHECK: [Error] Unexpected meta type. Expected HeapAllocation, but found StackAllocation
  // CHECK: [Error] Unexpected meta type. Expected StackAllocation, but found HeapAllocation
  // CHECK: [Trace] Freeing 1 stack entries...

  return 0;
}
