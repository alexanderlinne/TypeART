// RUN: echo --- > types.yaml
// RUN: %wrapper-cc -O1 %s -o %s.exe
// RUN: %s.exe 2>&1 | %filecheck %s

// RUN: %wrapper-cc -O1 -c %s -o %s.o
// RUN: %wrapper-cc %s.o -o %s.exe
// RUN: TYPEART_TYPE_FILE=%S/types.yaml %mpi-exec -np 1 %s.exe 2>&1 | %filecheck %s

#include "../../lib/runtime/tracker/CallbackInterface.h"

#include <stdio.h>

int main(int argc, char** argv) {
  int i;
  fprintf(stderr, "Expected pointer: %p\n", &i);
  return 0;
}

// CHECK: [Trace] Alloc
// CHECK: [Trace] Alloc
// CHECK: [Trace] Alloc
// CHECK: [Trace] Alloc [[PTR:0x[0-9a-f]+]] {{[0-9]*}} 2 int32 4 1
// CHECK: Expected pointer: [[PTR]]
