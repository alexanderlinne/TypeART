// RUN: %c-to-llvm %s | %apply-typeart -typeart-stack -S 2>&1 | %filecheck %s
// XFAIL: *

#include <stddef.h>
void typeart_tracker_leave_scope(size_t alloca_count);

int main(void) {
  typeart_tracker_leave_scope(0);
  return 0;
}