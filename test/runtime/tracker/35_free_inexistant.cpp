// clang-format off
// RUN: %run %s 2>&1 | %filecheck %s
// REQUIRES: tracker
// clang-format on

#include "util.hpp"

#include <stdio.h>

int main(int argc, char** argv) {
  const auto meta_id = create_fake_double_heap_allocation();
  const size_t extent{6};
  const size_t expected_count{extent};

  auto* d = new double[extent];

  // CHECK: [Trace]{{.*}}Free on nullptr
  typeart_tracker_free(nullptr);

  // CHECK-NOT: [Error]
  // CHECK-NOT: [Check]
  check(&d[0], "double", 6, false);

  // CHECK: [Trace] Free heap 0x{{[0-9a-f]+}} of type [6 x double]
  typeart_tracker_free(reinterpret_cast<const void*>(d));
  // CHECK: [Trace]{{.*}}Free on unregistered address
  delete[] d;

  return 0;
}
