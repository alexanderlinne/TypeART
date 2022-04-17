// RUN: %run  %s --compile_flags "-g" 2>&1 | %filecheck %s
// REQUIRES: tracker

#include <stdio.h>
#include <stdlib.h>
#include <typeart/TypeART.h>

struct Datastruct {
  int start;
  double middle;
  float end;
};

const void* check_addr(void* ptr) {
  typeart_pointer_info pointer_info;
  if (typeart_get_pointer_info(ptr, &pointer_info) != TYPEART_OK) {
    fprintf(stderr, "Error getting return address.\n");
    return NULL;
  }

  if (pointer_info.return_addr == NULL) {
    fprintf(stderr, "Error return address NULL.\n");
    return NULL;
  }

  fprintf(stderr, "Address check OK.\n");

  return pointer_info.return_addr;
}

int main(int argc, char** argv) {
  struct Datastruct d;

  const void* addr = check_addr(&d);
  if (addr == NULL) {
    return 1;
  }

  char* file;
  char* func;
  char* line;

  if (typeart_get_source_location(addr, &file, &func, &line) != TYPEART_OK) {
    fprintf(stderr, "Error getting source loc\n");
    return -1;
  }

  fprintf(stderr, "Loc File: %s\n", file);
  fprintf(stderr, "Loc Function: %s\n", func);
  fprintf(stderr, "Loc Line: %s\n", line);

  check_addr(file);
  check_addr(func);
  check_addr(line);

  free(file);
  free(line);
  free(func);

  return 0;
}

// CHECK-NOT: Error
// CHECK: Address check OK
// CHECK: Loc File:{{.*}}46_source_location.c
// CHECK: Loc Function: main
// CHECK: Loc Line: {{[0-9]+}}
// CHECK: Address check OK
// CHECK: Address check OK
// CHECK: Address check OK
