// clang-format off
// RUN: %cpp-to-llvm %s | %apply-typeart -S 2>&1 | %filecheck %s
// REQUIRES: tracker
// clang-format on

#include <new>

struct C {
  int x;
};

C* foo() {
  return new (std::nothrow) C[10];
}

C* bar() {
  return new (std::nothrow) C;
}

// CHECK: [[POINTER:%[0-9a-z]+]] = call noalias i8* @_ZnamRKSt9nothrow_t(i64{{( noundef)?}} 40,
// CHECK-NEXT: call void @typeart_tracker_alloc(i8* [[POINTER]], i32 [[ID:[0-9]*]], i64 10)

// CHECK: [[POINTER2:%[0-9a-z]+]] = call noalias i8* @_ZnwmRKSt9nothrow_t(i64{{( noundef)?}} 4,
// CHECK-NOT: call void @typeart_tracker_alloc(i8* [[POINTER2]], i32 [[ID]], i64 1)
// CHECK-NEXT: call void @typeart_tracker_alloc(i8* [[POINTER2]], i32 {{[0-9]*}}, i64 1)
