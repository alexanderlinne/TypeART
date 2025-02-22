// clang-format off
// RUN: %cpp-to-llvm %s | %apply-typeart -S 2>&1 | %filecheck %s
// REQUIRES: tracker
// clang-format on

struct alignas(16) S1 {
  int x;
  ~S1(){};
};

// CHECK: [[MEM:%[0-9a-z]+]] = call{{.*}} i8* @_Znam(i64{{( noundef)?}} 48)
// CHECK: [[ARR:%[0-9a-z]+]] = getelementptr inbounds i8, i8* [[MEM]], i64 16
// CHECK: call void @typeart_tracker_alloc(i8* [[ARR]], i32 {{[0-9]*}}, i64 2)
// CHECK: bitcast i8* [[ARR]] to %struct.S1*
int main() {
  S1* ss = new S1[2];
  return 0;
}

// CHECK: TypeArtPass [Heap & Stack]
// CHECK-NEXT: Malloc{{[ ]*}}:{{[ ]*}}1
// CHECK-NEXT: Free
// CHECK-NEXT: Alloca{{[ ]*}}:{{[ ]*}}0
