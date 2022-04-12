// clang-format off
// RUN: %c-to-llvm %s | %apply-typeart -S > %s.ll 2>&1
// RUN: cat %s.ll | %filecheck %s
// REQUIRES: tracker
// clang-format on
#include <stdlib.h>

void foo(int n) {
  int* pi = aligned_alloc(64, 20);

  int* pi2 = (int*)aligned_alloc(128, n);
}

// clang-format off
// CHECK: [[POINTER:%[0-9a-z]+]] = call noalias{{( align [0-9]+)?}} i8* @aligned_alloc(i64 64, i64 20)
// CHECK-NEXT: call void @typeart_tracker_alloc(i8* [[POINTER]], i32 2, i64 5)
// CHECK: bitcast i8* [[POINTER]] to i32*

// CHECK: [[POINTER2:%[0-9a-z]+]] = call noalias{{( align [0-9]+)?}} i8* @aligned_alloc(i64 128, i64 [[SIZE:%[0-9a-z]+]])
// CHECK-NOT: call void @typeart_tracker_alloc(i8* [[POINTER2]], i32 2, i64 [[SIZE]])
// CHECK: call void @typeart_tracker_alloc(i8* [[POINTER2]], i32 2, i64 %{{[0-9a-z]+}})
// CHECK: bitcast i8* [[POINTER2]] to i32*
// clang-format on
