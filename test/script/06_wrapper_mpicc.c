// RUN: echo --- > %s.types.yaml
// RUN: export TYPEART_TYPE_FILE="%s.types.yaml"
// RUN: %wrapper-mpicc -O1 %s -o %s.exe
// RUN: %mpi-exec -np 1 %s.exe 2>&1 | %filecheck %s

// RUN: %wrapper-mpicc -O1 -c %s -o %s.o
// RUN: %wrapper-mpicc %s.o -o %s.exe
// RUN: %mpi-exec -np 1 %s.exe 2>&1 | %filecheck %s

// REQUIRES: mpicc
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

// CHECK: R[0][Trace] Alloc [[PTR:0x[0-9a-f]+]] 2 int32 4 1
// CHECK: Expected pointer: [[PTR]]
