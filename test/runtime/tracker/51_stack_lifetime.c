// RUN: %run %s 2>&1 | %filecheck %s
// REQUIRES: tracker

#include <stdio.h>
#include <typeart/TypeART.h>

void type_check(const void* addr) {
  typeart_status status;
  typeart_pointer_info pointer_info;
  status = typeart_get_pointer_info(addr, &pointer_info);

  if (status != TYPEART_OK) {
    fprintf(stderr, "[Error]: Status not OK: %i for %p\n", status, addr);
  } else {
    fprintf(stderr, "Status OK: %i %zu\n", pointer_info.type_id, pointer_info.count);
  }
}

void correct(int rank) {
  if (rank == 1) {
    // CHECK: Status OK: 2 9
    // CHECK: Status OK: 2 8
    // CHECK: Status OK: 2 7
    // CHECK: Status OK: 2 1
    int buffer[3][3] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
    type_check(buffer);
    type_check(&buffer[0][1]);
    type_check(&buffer[0][2]);
    type_check(&buffer[2][2]);
  } else {
    // CHECK: Status OK: 2 3
    // CHECK: Status OK: 2 1
    int rcv[3] = {0, 1, 2};
    type_check(rcv);
    type_check(&rcv[2]);
  }
}

int main(void) {
  correct(1);
  correct(0);
  return 0;
}
