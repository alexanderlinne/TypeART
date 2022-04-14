// RUN: %run %s --manual 2>&1 | %filecheck %s
// REQUIRES: tracker

// TODO: typeart_tracker_alloc expects alloc id
// XFAIL: *

#include "runtime/tracker/CallbackInterface.h"

#include <stdlib.h>

int main(int argc, char** argv) {
  typeart_tracker_alloc((const void*)0, 6, 1);
  typeart_tracker_alloc((const void*)1, 5, 0);
  typeart_tracker_alloc((const void*)0, 6, 0);
  typeart_tracker_alloc((const void*)2, 7, 1);  // OK

  return 0;
}
// TODO disable Trace logs for early return?

// CHECK: [Error]{{.*}}:Nullptr allocation 0x0 6 double 8 1
// CHECK: [Warning]{{.*}}:Zero-size allocation 0x1 5 float 4 0
// CHECK: [Error]{{.*}}:Zero-size and nullptr allocation 0x0 6 double 8 0
// CHECK: [Trace] Alloc 0x2 7 float128 16 1