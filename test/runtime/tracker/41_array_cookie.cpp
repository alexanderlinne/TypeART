// RUN: %run %s 2>&1 | %filecheck %s
// REQUIRES: tracker

#include "util.hpp"

using namespace typeart;

struct S1 {
  int x;
  ~S1(){};
};

int main() {
  const auto check = [&](const void* addr, size_t elems) {
    auto pointer_info_result = runtime::PointerInfo::get(addr);
    if (pointer_info_result.has_error()) {
      fprintf(stderr, "[Check]: Status: %i with #elem %zu.\n", pointer_info_result.error(), elems);
      return;
    }
    auto pointer_info = pointer_info_result.value();
    if (pointer_info.getCount() != elems) {
      fprintf(stderr, "[Error]: Count not expected: %zu. Expected: %zu.\n", pointer_info.getCount(), elems);
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