// clang-format off
// RUN: %c-to-llvm %s | %apply-typeart -S 2>&1 | %filecheck %s
// REQUIRES: tracker
// clang-format on
#include <stdlib.h>
typedef struct ms {
  int a;
  double b;
} mystruct;

void test() {
  mystruct* m = (mystruct*)malloc(sizeof(mystruct));
  free(m);
}

// CHECK: [[POINTER:%[0-9a-z]+]] = call noalias{{( align [0-9]+)?}} i8* @malloc
// CHECK-NEXT: call void @typeart_tracker_alloc(i8* [[POINTER]], i32 {{[0-9]*}}, i64 1)
// CHECK-NEXT: bitcast i8* [[POINTER]] to %struct.ms*

// CHECK: call void @free(i8*{{( noundef)?}} [[POINTER:%[0-9a-z]+]])
// CHECK-NEXT: call void @typeart_tracker_free(i8* [[POINTER]])

// CHECK: TypeArtPass [Heap & Stack]
// CHECK-NEXT: Malloc{{[ ]*}}:{{[ ]*}}1
// CHECK-NEXT: Free{{[ ]*}}:{{[ ]*}}1
// CHECK-NEXT: Alloca{{[ ]*}}:{{[ ]*}}0
