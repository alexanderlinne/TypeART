// clang-format off
// RUN: %cpp-to-llvm %s | %apply-typeart -S 2>&1 | %filecheck %s
// REQUIRES: tracker
// clang-format on

struct S1 {
  int x;
  ~S1() = default;
};

// CHECK: call{{.*}} i8* @_Znam(i64{{( noundef)?}} 8)
// CHECK: call void @typeart_tracker_alloc(i8* [[POINTER:%[0-9a-z]+]], i32 {{[0-9]*}}, i64 2)
// CHECK: bitcast i8* [[POINTER]] to %struct.S1*
int main() {
  S1* ss = new S1[2];
  return 0;
}

// CHECK: TypeArtPass [Heap & Stack]
// CHECK-NEXT: Malloc{{[ ]*}}:{{[ ]*}}1
// CHECK-NEXT: Free
// CHECK-NEXT: Alloca{{[ ]*}}:{{[ ]*}}0
