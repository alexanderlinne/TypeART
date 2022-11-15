// RUN: %run %s 2>&1 | %filecheck %s
// REQUIRES: tracker
#include <stdlib.h>

int main(int argc, char** argv) {
  const int n = 42;
  // CHECK: [Trace] TypeART Runtime Trace

  // CHECK: [Trace] Alloc stack 0x{{.*}} of type [1 x unsigned char[42]]
  unsigned char a[n];

  // CHECK: [Trace] Alloc stack 0x{{.*}} of type [1 x unsigned short[42]]
  unsigned short b[n];

  // CHECK: [Trace] Alloc stack 0x{{.*}} of type [1 x unsigned int[42]]
  unsigned int c[n];

  // CHECK: [Trace] Alloc stack 0x{{.*}} of type [1 x long unsigned int[42]]
  unsigned long d[n];

  // CHECK: [Trace] Free stack 0x{{.*}} of type [1 x unsigned char[42]]
  // CHECK: [Trace] Free stack 0x{{.*}} of type [1 x unsigned short[42]]
  // CHECK: [Trace] Free stack 0x{{.*}} of type [1 x unsigned int[42]]
  // CHECK: [Trace] Free stack 0x{{.*}} of type [1 x long unsigned int[42]]

  return 0;
}
