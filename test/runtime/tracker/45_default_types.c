// RUN: %run %s --clean_types 2>&1 | %filecheck %s
// REQUIRES: tracker

// TODO: with alloc_ids the runtime can't work without a proper types file
// XFAIL: *

#include <stdlib.h>

int main(int argc, char** argv) {
  const int n = 42;
  // CHECK: [Trace] TypeART Runtime Trace
  // CHECK: [Warning]{{.*}}No type file with default name
  // CHECK: [Trace] Alloc 0x{{.*}} int8 1 42
  char* a = malloc(n * sizeof(char));
  // CHECK: [Trace] Free 0x{{.*}}
  free(a);
}
