// RUN: %run %s 2>&1 | %filecheck %s
// REQUIRES: tracker

#include "util.h"

struct S1 {
  int x;
  ~S1(){};
};

int main() {
  const auto check = [&](auto* addr, size_t elems) {
    typeart_pointer_info pointer_info;
    typeart_status status = typeart_get_pointer_info(reinterpret_cast<const void*>(addr), &pointer_info);

    if (status == TYPEART_OK) {
      if (pointer_info.count != elems) {
        fprintf(stderr, "[Error]: Count not expected: %zu. Expected: %zu.\n", pointer_info.count, elems);
      }
    } else {
      fprintf(stderr, "[Check]: Status: %i with #elem %zu.\n", status, elems);
    }
  };

  for (size_t elems = 1; elems < 5; ++elems) {
    // allocates additional sizeof(*size_t*) bytes to store expected count -> array cookie:
    S1* ss = new S1[elems];
    check(ss, elems);
    delete[] ss;
  }

  return 0;
}

// CHECK-NOT: Error
// CHECK-NOT: [Check]: Status: {{[1-9]+}}