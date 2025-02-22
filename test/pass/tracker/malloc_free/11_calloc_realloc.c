// clang-format off
// RUN: %c-to-llvm %s | %apply-typeart -S 2>&1 | %filecheck %s --check-prefix=REALLOC
// RUN: %c-to-llvm %s | %apply-typeart -S 2>&1 | %filecheck %s
// REQUIRES: tracker
// clang-format on
#include <stdlib.h>

int main() {
  double* pd = calloc(10, sizeof(double));

  pd = realloc(pd, 20 * sizeof(double));

  return 0;
}

// CHECK: [[POINTER:%[0-9a-z]+]] = call noalias{{( align [0-9]+)?}} i8* @calloc(i64{{( noundef)?}} [[SIZE:[0-9]+]], i64{{( noundef)?}} 8)
// CHECK-NEXT: call void @typeart_tracker_alloc(i8* [[POINTER]], i32 {{[0-9]*}}, i64 [[SIZE]])
// CHECK-NEXT: bitcast i8* [[POINTER]] to double*

// REALLOC: typeart_tracker_free(i8* [[POINTER:%[0-9a-z]+]])
// REALLOC-NEXT: [[POINTER2:%[0-9a-z]+]] = call{{( align [0-9]+)?}} i8* @realloc(i8*{{( noundef)?}} [[POINTER]], i64{{( noundef)?}} 160)
// REALLOC-NEXT: typeart_tracker_alloc(i8* [[POINTER2]], i32 {{[0-9]*}}, i64 20)

// CHECK: TypeArtPass [Heap & Stack]
// CHECK-NEXT: Malloc{{[ ]*}}:{{[ ]*}}2
// CHECK-NEXT: Free{{[ ]*}}:{{[ ]*}}0
// CHECK-NEXT: Alloca{{[ ]*}}:{{[ ]*}}0

// clang-format on
