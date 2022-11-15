// clang-format off
// RUN: %c-to-llvm %s > %s.ll
// RUN: cat %s.ll | %apply-typeart -S | %apply-typeart -typeart-stack -typeart-heap=false -typeart-call-filter -S > %s.log 2>&1
// RUN: cat %s.log | %filecheck %s
// REQUIRES: tracker
// clang-format on

#include <stdlib.h>

double* a;

int main(int argc, char** argv) {
  int n = argc * 2;

  a = (double*)malloc(sizeof(double) * n);

  free(a);

  return 0;
}

// CHECK: call void @typeart_tracker_alloc(
// CHECK: call void @typeart_tracker_free(

// CHECK-NOT: call void @typeart_tracker_leave_scope

// CHECK:      TypeArtPass [Heap & Stack]
// CHECK-NEXT  Malloc :   0
// CHECK-NEXT  Free   :   0
// CHECK-NEXT  Alloca :   0
// CHECK-NEXT  Global :   0