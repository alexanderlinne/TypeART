// RUN: %run %s -typeart-stack-array-only=true 2>&1 | %filecheck %s
// REQUIRES: tracker

#include "../../struct_defs.h"
#include "util.hpp"

#include <stdint.h>
#include <stdlib.h>

int main(int argc, char** argv) {
  s_int s;

  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [1 x s_ptr_to_self_t]
  s_ptr_to_self_t* a = (s_ptr_to_self_t*)malloc(sizeof(s_ptr_to_self_t));
  // CHECK: Ok
  check_struct(a, "s_ptr_to_self_t", true);
  // CHECK: Ok
  check(a, "s_ptr_to_self_t", 1, false);
  // CHECK: Ok
  check(a, "s_ptr_to_self_t*", 1, true);
  // CHECK: Ok
  check(&a->b, "s_ptr_to_self_t**", 1, true);
  // CHECK: Error: Unknown address
  check(a + 1, "s_ptr_to_self_t", 1, false);
  // CHECK: [Trace] Free heap 0x{{.*}}
  free(a);

  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [1 x s_struct_member_t]
  s_struct_member_t* b = (s_struct_member_t*)malloc(sizeof(s_struct_member_t));
  // CHECK: Ok
  check_struct(b, "s_struct_member_t", true);
  // CHECK: Ok
  check(b, "s_struct_member_t", 1, false);
  // CHECK: Ok
  check(b, "int", 1, true);
  // CHECK: Ok
  check(&b->b, "s_ptr_to_self_t", 1, false);
  // CHECK: Ok
  check(&b->b, "s_ptr_to_self_t*", 1, true);
  // CHECK: Ok
  check(&b->c, "s_struct_member_t*", 1, true);
  // CHECK: Error: Unknown address
  check(b + 1, "int", 1, true);
  // CHECK: [Trace] Free heap 0x{{.*}}
  free(b);

  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [1 x s_aos_t]
  s_aos_t* c = (s_aos_t*)malloc(sizeof(s_aos_t));
  // CHECK: Ok
  check_struct(c, "s_aos_t", true);
  // CHECK: Ok
  check(c, "s_aos_t", 1, false);
  // CHECK: Ok
  check(c, "int", 1, true);
  // CHECK: Ok
  check(&c->b, "s_struct_member_t[2]", 1, false);
  // CHECK: Ok
  check(&c->b[1], "s_struct_member_t", 1, false);
  // CHECK: Ok
  check(&c->c, "s_struct_member_t*", 3, true);
  // CHECK: Error: Unknown address
  check(c + 1, "int", 1, true);
  // CHECK: [Trace] Free heap 0x{{.*}}
  free(c);

  return 0;
}
