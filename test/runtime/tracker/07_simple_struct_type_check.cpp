// RUN: %run %s -typeart-stack-array-only=true > %s.log 2>&1
// RUN: cat %s.log | %filecheck %s
// REQUIRES: tracker

#include "../../struct_defs.h"
#include "util.hpp"

#include <stdint.h>
#include <stdlib.h>

int main(int argc, char** argv) {
  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [1 x s_int_t]
  s_int_t* a = (s_int_t*)malloc(sizeof(s_int_t));
  // CHECK: Ok
  check_struct(a, "s_int_t", true);
  // CHECK: Ok
  check(a, "s_int_t", 1, false);
  // CHECK: Ok
  check(a, "int", 1, true);
  // CHECK: Error: Unknown address
  check(a + 1, "int", 1, true);
  // CHECK: [Trace] Free heap 0x{{.*}}
  free(a);

  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [1 x s_builtins_t]
  s_builtins_t* b = (s_builtins_t*)malloc(sizeof(s_builtins_t));
  // CHECK: Ok
  check_struct(b, "s_builtins_t", true);
  // CHECK: Ok
  check(b, "s_builtins_t", 1, false);
  // CHECK: Ok
  check(b, "int", 1, true);
  // CHECK: Error: Type mismatch
  check(b, "char", 1, true);
  // CHECK: Error: Bad alignment
  check(((uint8_t*)b) + 2, "int", 1, true);
  // CHECK: Ok
  check(&b->b, "char", 1, true);
  // CHECK: Error: Bad alignment
  check(((uint8_t*)b) + 5, "long int", 1, true);
  // CHECK: Ok
  check(&b->c, "long int", 1, true);
  // CHECK: Error: Unknown address
  check(b + 1, "s_builtins_t", 1, false);
  // CHECK: [Trace] Free heap 0x{{.*}}
  free(b);

  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [1 x s_arrays_t]
  s_arrays_t* c = (s_arrays_t*)malloc(sizeof(s_arrays_t));
  // CHECK: Ok
  check_struct(c, "s_arrays_t", true);
  // CHECK: Ok
  check(c, "s_arrays_t", 1, false);
  // CHECK: Ok
  check(c, "int", 3, true);
  // CHECK: Ok
  check(((uint8_t*)c) + 4, "int", 2, true);
  // CHECK: Ok
  check(((uint8_t*)c) + 8, "int", 1, true);
  // CHECK: Bad alignment
  check(((uint8_t*)c) + 12, "int", 2, true);
  // CHECK: Ok
  check(&c->b, "long int", 2, true);
  // CHECK: Ok
  check(&c->b[1], "long int", 1, true);
  // CHECK: Ok
  check(&c->e[2], "char", 3, true);
  // CHECK: Error: Unknown address
  check(c + 1, "s_arrays_t", 1, false);
  // CHECK: [Trace] Free heap 0x{{.*}}
  free(c);

  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [1 x s_ptrs_t]
  s_ptrs_t* d = (s_ptrs_t*)malloc(sizeof(s_ptrs_t));
  // CHECK: Ok
  check_struct(d, "s_ptrs_t", true);
  // CHECK: Ok
  check(d, "s_ptrs_t", 1, false);
  // CHECK: Ok
  check(d, "char", 1, true);
  // CHECK: Ok
  check(&d->b, "int*", 1, true);
  // CHECK: Bad alignment
  check(((uint8_t*)d) + 12, "int*", 1, true);
  // CHECK: Ok
  check(&d->d, "double*", 1, true);
  // CHECK: Error: Unknown address
  check(d + 1, "s_ptrs_t", 1, false);
  // CHECK: [Trace] Free heap 0x{{.*}}
  free(d);

  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [1 x s_mixed_simple_t]
  s_mixed_simple_t* e = (s_mixed_simple_t*)malloc(sizeof(s_mixed_simple_t));
  // CHECK: Ok
  check_struct(e, "s_mixed_simple_t", true);
  // CHECK: Ok
  check(e, "s_mixed_simple_t", 1, false);
  // CHECK: Ok
  check(e, "int", 1, true);
  // CHECK: Ok
  check(((uint8_t*)e) + 16, "double", 2, true);
  // CHECK: Ok
  check(&e->c, "char*", 1, true);
  // CHECK: Error: Unknown address
  check(e + 1, "s_mixed_simple_t", 1, false);
  // CHECK: [Trace] Free heap 0x{{.*}}
  free(e);

  return 0;
}
