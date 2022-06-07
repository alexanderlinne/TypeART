// clang-format off
// RUN: %run %s -typeart-filter-pointer-alloca=false 2>&1 | %filecheck %s
// RUN: %run %s "-typeart-call-filter" 2>&1 | %filecheck %s --check-prefix CHECK-FILTER
// REQUIRES: tracker
// clang-format on

#include <stdlib.h>

void call() {
  const int n = 10;
  double a[n];
  double b[n];
}

int main(int argc, char** argv) {
  const int n = 42;
  // CHECK: [Trace] TypeART Runtime Trace
  // CHECK-FILTER-NOT: [Trace] TypeART Runtime Trace
  // CHECK-FILTER-NOT [Trace]

  // CHECK: [Trace] Alloc stack 0x{{.*}} of type [1 x char[42]]
  char a[n];

  // CHECK: [Trace] Alloc stack 0x{{.*}} of type [1 x short[42]]
  short b[n];

  // CHECK: [Trace] Alloc stack 0x{{.*}} of type [1 x int[42]]
  int c[n];

  // CHECK: [Trace] Alloc stack 0x{{.*}} of type [1 x long int[42]]
  long d[n];

  // CHECK: [Trace] Alloc stack 0x{{.*}} of type [1 x float[42]]
  float e[n];

  // CHECK: [Trace] Alloc stack 0x{{.*}} of type [1 x double[42]]
  double f[n];

  // CHECK: [Trace] Alloc stack 0x{{.*}} of type [1 x int*[42]]
  int* g[n];

  // CHECK: [Trace] Alloc stack 0x{{.*}} of type [1 x double[10]]
  // CHECK: [Trace] Alloc stack 0x{{.*}} of type [1 x double[10]]
  call();
  // CHECK: [Trace] Free stack 0x{{.*}} of type [1 x double[10]]
  // CHECK: [Trace] Free stack 0x{{.*}} of type [1 x double[10]]

  // CHECK: [Trace] Free stack 0x{{.*}} of type [1 x char[42]]
  // CHECK: [Trace] Free stack 0x{{.*}} of type [1 x short[42]]
  // CHECK: [Trace] Free stack 0x{{.*}} of type [1 x int[42]]
  // CHECK: [Trace] Free stack 0x{{.*}} of type [1 x long int[42]]
  // CHECK: [Trace] Free stack 0x{{.*}} of type [1 x float[42]]
  // CHECK: [Trace] Free stack 0x{{.*}} of type [1 x double[42]]
  // CHECK: [Trace] Free stack 0x{{.*}} of type [1 x int*[42]]

  return 0;
}
