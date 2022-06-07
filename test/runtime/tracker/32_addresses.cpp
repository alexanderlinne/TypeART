// RUN: %run %s 2>&1 | %filecheck %s
// REQUIRES: tracker

#include "util.hpp"

#include <stdlib.h>

using namespace typeart;

int main(int argc, char** argv) {
  auto alloc_id = create_fake_double_heap_alloc_id().value();

  typeart_tracker_alloc((const void*)0, alloc_id, 1);
  typeart_tracker_alloc((const void*)1, alloc_id, 0);
  typeart_tracker_alloc((const void*)0, alloc_id, 0);
  typeart_tracker_alloc((const void*)2, alloc_id, 1);  // OK

  return 0;
}
// TODO disable Trace logs for early return?

// CHECK: [Error] Nullptr allocation 0x0 of type [1 x double]
// CHECK: [Warning] Zero-size allocation 0x1 of type [0 x double]
// CHECK: [Error] Zero-size and nullptr allocation 0x0 of type [0 x double]
// CHECK: [Trace] Alloc heap 0x{{.*}} of type [1 x double]