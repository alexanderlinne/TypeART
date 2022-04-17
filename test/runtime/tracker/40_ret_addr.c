// RUN: %run %s --manual 2>&1 | %filecheck %s
// REQUIRES: tracker

#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <typeart/TypeART.h>

int main(int argc, char** argv) {
  const void* addr = 1;

  typeart_pointer_info pointer_info;
  typeart_get_pointer_info(addr, &pointer_info);
  if (pointer_info.return_addr != NULL) {
    fprintf(stderr, "[Error] Ret address mismatch expected NULL but have %p\n", pointer_info.return_addr);
  }
  return 0;
}

// CHECK-NOT: [Error]
