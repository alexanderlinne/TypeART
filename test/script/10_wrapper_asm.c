// RUN: echo --- > %s.types.yaml
// RUN: export TYPEART_TYPE_FILE="%s.types.yaml"
// RUN: %wrapper-cc -S -O1 %s -o %s.s
// RUN: cat %s.s 2>&1 | %filecheck %s

// RUN: %wrapper-cc -fPIC -S -O1 %s -o %s.s
// RUN: cat %s.s 2>&1 | %filecheck %s

// RUN: TYPEART_WRAPPER=OFF %wrapper-cc -S -O1 %s -o %s-van.s
// RUN: cat %s-van.s 2>&1 | %filecheck %s --check-prefixes vanilla-CHECK

#include <stdlib.h>

int main(int argc, char** argv) {
  int* p = malloc(argc * sizeof(int));
  return 0;
}

// CHECK: call{{.*}}typeart_tracker_alloc
// vanilla-CHECK-NOT: call{{.*}}typeart_tracker_alloc
