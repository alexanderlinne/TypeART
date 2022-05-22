// RUN: %run %s 2>&1 | %filecheck %s
// REQUIRES: tracker
#include <stdlib.h>

int main(int argc, char** argv) {
  const int n = 42;
  // CHECK: [Trace] TypeART Runtime Trace

  // CHECK: [Trace] Alloc stack 0x{{.*}} of type [42 x unsigned char]
  unsigned char a[n];

  // CHECK: [Trace] Alloc stack 0x{{.*}} of type [42 x unsigned short]
  unsigned short b[n];

  // CHECK: [Trace] Alloc stack 0x{{.*}} of type [42 x unsigned int]
  unsigned int c[n];

  // CHECK: [Trace] Alloc stack 0x{{.*}} of type [42 x long unsigned int]
  unsigned long d[n];

  // CHECK: [Trace] Free stack 0x{{.*}} of type [42 x unsigned char]
  // CHECK: [Trace] Free stack 0x{{.*}} of type [42 x unsigned short]
  // CHECK: [Trace] Free stack 0x{{.*}} of type [42 x unsigned int]
  // CHECK: [Trace] Free stack 0x{{.*}} of type [42 x long unsigned int]

  return 0;
}
