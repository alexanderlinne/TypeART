// RUN: %run %s > %s.log 2>&1
// RUN: cat %s.log | %filecheck %s
// REQUIRES: tracker

#include <stdlib.h>

int main(int argc, char** argv) {
  const int n = 42;
  // CHECK: [Trace] TypeART Runtime Trace

  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [42 x char]
  char* a = malloc(n * sizeof(char));
  // CHECK: [Trace] Free heap 0x{{.*}} of type [42 x char]
  free(a);

  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [42 x short]
  short* b = malloc(n * sizeof(short));
  // CHECK: [Trace] Free heap 0x{{.*}} of type [42 x short]
  free(b);

  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [42 x int]
  int* c = malloc(n * sizeof(int));
  // CHECK: [Trace] Free heap 0x{{.*}} of type [42 x int]
  free(c);

  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [42 x long int]
  long* d = malloc(n * sizeof(long));
  // CHECK: [Trace] Free heap 0x{{.*}} of type [42 x long int]
  free(d);

  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [42 x float]
  float* e = malloc(n * sizeof(float));
  // CHECK: [Trace] Free heap 0x{{.*}} of type [42 x float]
  free(e);

  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [42 x double]
  double* f = malloc(n * sizeof(double));
  // CHECK: [Trace] Free heap 0x{{.*}} of type [42 x double]
  free(f);

  return 0;
}
