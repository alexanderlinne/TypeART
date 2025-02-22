// clang-format off
// RUN: %c-to-llvm %s | %apply-typeart -S 2>&1 | %filecheck %s
// REQUIRES: tracker
// clang-format on
#include <stdlib.h>

void foo(double* ptr);

void test() {
  double* p = (double*)malloc(42 * sizeof(double));
  foo(p);
}

void foo(double* ptr) {
  free(ptr);
  ptr = NULL;
}

// CHECK: [[POINTER:%[0-9a-z]+]] = call noalias{{( align [0-9]+)?}} i8* @malloc
// CHECK-NEXT: call void @typeart_tracker_alloc(i8* [[POINTER]],
// CHECK-NEXT: bitcast i8* [[POINTER]] to double*

// CHECK: call void @free(i8*{{( noundef)?}} [[POINTER:%[0-9a-z]+]])
// CHECK-NEXT: call void @typeart_tracker_free(i8* [[POINTER]])

// CHECK: TypeArtPass [Heap & Stack]
// CHECK-NEXT: Malloc{{[ ]*}}:{{[ ]*}}1
// CHECK-NEXT: Free{{[ ]*}}:{{[ ]*}}1
// CHECK-NEXT: Alloca{{[ ]*}}:{{[ ]*}}0
