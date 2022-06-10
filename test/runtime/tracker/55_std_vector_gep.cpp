// RUN: %run %s 2>&1 | %filecheck %s
// XFAIL: *

// Currently fails because call instructions are not handled by
// backtrackDIType.

// The call to operator[] becomes a gep with 5 indices when
// optimizing with -O2. This becomes relevant for instrumenting
// the heap after optimization.
// With opaque pointers and -O2 only an i32 gep with a single
// index remains.

#include <vector>

void store_to_vec(std::vector<int*>& vec) {
  vec[3] = new int;
}

int main() {
  auto vec = std::vector<int*>{};
  vec.resize(4);
  store_to_vec(vec);
}
