// clang-format off
// RUN: %cpp-to-llvm %s | %apply-typeart -S 2>&1 | %filecheck %s
// REQUIRES: tracker
// clang-format on

#include <new>
// CHECK: invoke{{.*}} i8* @_Znam(i64{{( noundef)?}} 8)
// CHECK: call void @typeart_tracker_alloc(i8* [[POINTER:%[0-9a-z]+]], i32 {{[0-9]*}}, i64 2)
// CHECK-NEXT: bitcast i8* {{.*}}[[POINTER]] to i32*
int main() {
  try {
    auto s = new int[2];
  } catch (...) {
  }

  return 0;
}

// CHECK: TypeArtPass [Heap & Stack]
// CHECK-NEXT: Malloc{{[ ]*}}:{{[ ]*}}1
// CHECK-NEXT: Free
// CHECK-NEXT: Alloca{{[ ]*}}:{{[ ]*}}0
