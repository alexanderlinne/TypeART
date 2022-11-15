// RUN: %run %s 2>&1 | %filecheck %s
// REQUIRES: tracker
#include <stdlib.h>

int main(int argc, char** argv) {
  const int n = 42;
  // CHECK: [Trace] TypeART Runtime Trace

  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [42 x unsigned char]
  unsigned char* a = (unsigned char*)malloc(n * sizeof(unsigned char));
  // CHECK: [Trace] Free heap 0x{{.*}} of type [42 x unsigned char]
  free(a);

  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [42 x unsigned short]
  unsigned short* b = (unsigned short*)malloc(n * sizeof(unsigned short));
  // CHECK: [Trace] Free heap 0x{{.*}} of type [42 x unsigned short]
  free(b);

  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [42 x unsigned int]
  unsigned int* c = (unsigned int*)malloc(n * sizeof(unsigned int));
  // CHECK: [Trace] Free heap 0x{{.*}} of type [42 x unsigned int]
  free(c);

  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [42 x long unsigned int]
  unsigned long* d = (unsigned long*)malloc(n * sizeof(unsigned long));
  // CHECK: [Trace] Free heap 0x{{.*}} of type [42 x long unsigned int]
  free(d);

  return 0;
}
