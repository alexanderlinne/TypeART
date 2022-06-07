// RUN: %run %s 2>&1 | %filecheck %s
// REQUIRES: tracker

#include "util.hpp"

#include <iostream>
#include <stdlib.h>

using namespace typeart;

int main(int argc, char** argv) {
  const int addr      = 2;
  const auto alloc_id = create_inexistent_meta_alloc_id();
  const size_t extent = 2;
  typeart_tracker_alloc((const void*)addr, alloc_id.value(), extent);

  auto pointer_info_result = runtime::PointerInfo::get((const void*)addr);

  if (pointer_info_result.has_error()) {
    std::cerr << "[Error] Status not OK: " << pointer_info_result.error() << "\n";
  } else {
    std::cerr << "[Trace] Status OK unexpectedly\n";
  }
  return 0;
}

// CHECK: [Error] Allocation info with unknown meta_id! Skipping...
// CHECK: [Error] Status not OK: UNKNOWN_ADDRESS
