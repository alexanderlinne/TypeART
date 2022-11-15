// clang-format off
// RUN: %c-to-llvm %s | %apply-typeart -S | %filecheck %s
// RUN: %c-to-llvm %s | %apply-typeart -S 2>&1 | %filecheck %s --check-prefix=PASS-OUT
// REQUIRES: tracker
// clang-format on
#include <stdlib.h>
void test() {
  void* p   = malloc(42 * sizeof(int));
  int* pi   = (int*)p;
  short* ps = (short*)p;
}

// CHECK: [[POINTER:%[0-9a-z]+]] = call noalias{{( align [0-9]+)?}} i8* @malloc
// CHECK-NEXT: call void @typeart_tracker_alloc(i8* [[POINTER]],

// PASS-OUT: [Warning]{{.*}}Encountered ambiguous pointer type in function

// PASS-OUT: TypeArtPass [Heap & Stack]
// PASS-OUT-NEXT: Malloc{{[ ]*}}:{{[ ]*}}1
// PASS-OUT-NEXT: Free{{[ ]*}}:{{[ ]*}}0
// PASS-OUT-NEXT: Alloca{{[ ]*}}:{{[ ]*}}0
