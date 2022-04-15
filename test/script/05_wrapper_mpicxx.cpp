// RUN: echo --- > types.yaml
// RUN: %wrapper-mpicxx -g %s -o %s.exe
// RUN: %mpi-exec -np 1 %s.exe 2>&1 | %filecheck %s

// RUN: %wrapper-mpicxx -g -c %s -o %s.o
// RUN: %wrapper-mpicxx %s.o -o %s.exe
// RUN: TYPEART_TYPE_FILE=%S/types.yaml %mpi-exec -np 1 %s.exe 2>&1 | %filecheck %s

// REQUIRES: mpicxx
// UNSUPPORTED: sanitizer

#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);
  int i;
  fprintf(stderr, "Expected pointer: %p\n", &i);
  MPI_Bsend(&i, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
  MPI_Finalize();
  return 0;
}

// CHECK: R[0][Trace] Alloc [[PTR:0x[0-9a-f]+]] {{[0-9]*}} 2 int32 4 1
// CHECK: Expected pointer: [[PTR]]
