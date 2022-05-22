// RUN: %run %s -typeart-filter-pointer-alloca=false 2>&1 | %filecheck %s
// REQUIRES: tracker

#include <stdlib.h>

int main(int argc, char** argv) {
  const int n = 42;
  // CHECK: [Trace] TypeART Runtime Trace

  // CHECK: [Trace] Alloc stack 0x{{.*}} of type [42 x char]
  char a[n];

  // CHECK: [Trace] Alloc stack 0x{{.*}} of type [42 x short]
  short b[n];

  // CHECK: [Trace] Alloc stack 0x{{.*}} of type [42 x int]
  int c[n];

  // CHECK: [Trace] Alloc stack 0x{{.*}} of type [42 x long int]
  long d[n];

  // CHECK: [Trace] Alloc stack 0x{{.*}} of type [42 x float]
  float e[n];

  // CHECK: [Trace] Alloc stack 0x{{.*}} of type [42 x double]
  double f[n];

  // CHECK: [Trace] Alloc stack 0x{{.*}} of type [42 x int*]
  int* g[n];

  // CHECK: [Trace] Alloc stack 0x{{.*}} of type [1764 x int]
  int h[n][n];

  // CHECK: [Trace] Alloc stack 0x{{.*}} of type [74088 x int]
  int i[n][n][n];

  // CHECK: [Trace] Free stack 0x{{.*}} of type [42 x char]
  // CHECK: [Trace] Free stack 0x{{.*}} of type [42 x short]
  // CHECK: [Trace] Free stack 0x{{.*}} of type [42 x int]
  // CHECK: [Trace] Free stack 0x{{.*}} of type [42 x long int]
  // CHECK: [Trace] Free stack 0x{{.*}} of type [42 x float]
  // CHECK: [Trace] Free stack 0x{{.*}} of type [42 x double]
  // CHECK: [Trace] Free stack 0x{{.*}} of type [42 x int*]
  // CHECK: [Trace] Free stack 0x{{.*}} of type [1764 x int]
  // CHECK: [Trace] Free stack 0x{{.*}} of type [74088 x int]

  return 0;
}
