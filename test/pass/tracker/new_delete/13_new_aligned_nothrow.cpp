// clang-format off
// RUN: %cpp-to-llvm %s | %apply-typeart -S 2>&1 | %filecheck %s
// REQUIRES: tracker
// clang-format on

#define __cpp_aligned_new 1
#include <new>

struct C {
  int x;
};

C* foo() {
  return new (std::align_val_t(64), std::nothrow) C[10];
}

C* bar() {
  return new (std::align_val_t(128), std::nothrow) C;
}

// CHECK: [[POINTER:%[0-9a-z]+]] = call noalias i8* @_ZnamSt11align_val_tRKSt9nothrow_t(i64 40,
// CHECK-NEXT: call void @typeart_tracker_alloc(i8* [[POINTER]], i32 [[ID:[0-9]*]], i64 10)

// CHECK: [[POINTER2:%[0-9a-z]+]] = call noalias i8* @_ZnwmSt11align_val_tRKSt9nothrow_t(i64 4,
// CHECK-NOT: call void @typeart_tracker_alloc(i8* [[POINTER2]], i32 [[ID]], i64 1)
// CHECK-NEXT: call void @typeart_tracker_alloc(i8* [[POINTER2]], i32 {{[0-9]*}}, i64 1)