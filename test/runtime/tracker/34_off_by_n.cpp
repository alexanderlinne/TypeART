// clang-format off
// RUN: %run %s 2>&1 | %filecheck %s
// REQUIRES: tracker
// clang-format on

#include "util.hpp"

#include <stdio.h>

int main(int argc, char** argv) {
  const auto alloc_id = create_fake_double_heap_alloc_id();
  const auto extent   = 6;
  const size_t expected_count{1};

  auto* d = new double[extent];

  typeart_tracker_alloc(reinterpret_cast<const void*>(&d[0]), alloc_id.value(), 1);
  typeart_tracker_alloc(reinterpret_cast<const void*>(&d[1]), alloc_id.value(), 1);

  // CHECK-NOT: [Error]
  check(&d[0], "double", 1, false);
  check(&d[1], "double", 1, false);
  // clang-format off
  // CHECK: [Warning] {{.*}} was 1 elements out of bounds!
  // clang-format on
  // CHECK: Error: Unknown address
  check(&d[2], "double", 1, false);  // one off
  // clang-format off
  // CHECK: [Warning] {{.*}} was 4 elements out of bounds!
  // clang-format on
  // CHECK: Error: Unknown address
  check(&d[5], "double", 1, false);  // four off

  // CHECK-NOT: out of bounds
  // CHECK-NOT: [Error]
  // CHECK: Error: Unknown address
  double* p_0 = (&d[0]) - 1;
  check(p_0, "double", 1, false);  // -1 off

  delete[] d;

  return 0;
}
