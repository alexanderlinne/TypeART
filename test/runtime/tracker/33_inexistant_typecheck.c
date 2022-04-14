// RUN: %run %s --manual 2>&1 | %filecheck %s
// REQUIRES: tracker

// TODO: how to handle this test with alloc_ids?
// XFAIL: *

#include "runtime/tracker/CallbackInterface.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
  const int addr      = 2;
  const int type      = -1;
  const size_t extent = 2;
  typeart_tracker_alloc((const void*)addr, type, extent);

  int id_result         = 0;
  size_t count_check    = 0;
  typeart_status status = typeart_get_type((const void*)addr, &id_result, &count_check);

  if (status != TYPEART_OK) {
    fprintf(stderr, "[Error]: Status not OK: %i\n", status);
  } else {
    if (extent != count_check) {
      fprintf(stderr, "[Error]: Count check failed %zu\n", count_check);
    }
    if (type != id_result) {
      fprintf(stderr, "[Error]: ID check failed %i\n", id_result);
    }
    fprintf(stderr, "[Trace]: Status OK: %i %zu\n", id_result, count_check);
  }
  return 0;
}

// TODO the runtime continues, even if type is unknown.

// CHECK: [Error]{{.*}}Allocation of unknown type 0x2 -1 typeart_unknown_struct 0 2
// CHECK: [Trace] Alloc 0x2 -1 typeart_unknown_struct 0 2
// CHECK: Status OK: -1 2
