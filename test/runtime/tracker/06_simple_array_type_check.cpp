// RUN: %run %s 2>&1 | %filecheck %s
// REQUIRES: tracker

#include "util.hpp"

#include <stdint.h>
#include <stdlib.h>
#include <typeart/TypeART.hpp>

template <typename T>
void performTypeChecks(int n, const char* type_name) {
  T* p = (T*)malloc(n * sizeof(T));
  check(p - 1, type_name, 1, true);                                    // Unknown address
  check(p, type_name, n, true);                                        // Ok
  check(p + n / 2, type_name, n - n / 2, true);                        // Ok
  check(p + n - 1, type_name, 1, true);                                // Ok
  check(p + n, type_name, 1, true);                                    // Error: Unknown address
  check(((uint8_t*)p) + 1, type_name, n - 1, true);                    // Fails for sizeof(T) > 1
  check(((uint8_t*)(p + n / 2)) + 1, type_name, n - n / 2 - 1, true);  // Fails for sizeof(T) > 1
  check(((uint8_t*)p) + 2, type_name, n - 2 / sizeof(T), true);        // Fails for sizeof(T) > 2
  check(((uint8_t*)p) + 4, type_name, n - 4 / sizeof(T), true);        // Fails for sizeof(T) > 4
  check(((uint8_t*)p) + 8, type_name, n - 8 / sizeof(T), true);        // Fails for sizeof(T) > 8
  free(p);
}

int main(int argc, char** argv) {
  const int n = 42;

  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [42 x char]
  // CHECK: Error: Unknown address
  // CHECK: Ok
  // CHECK: Ok
  // CHECK: Ok
  // CHECK: Error: Unknown address
  // CHECK: Ok
  // CHECK: Ok
  // CHECK: Ok
  // CHECK: Ok
  // CHECK: Ok
  // CHECK: [Trace] Free heap 0x{{.*}}
  performTypeChecks<char>(n, "char");

  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [42 x short]
  // CHECK: Error: Unknown address
  // CHECK: Ok
  // CHECK: Ok
  // CHECK: Ok
  // CHECK: Error: Unknown address
  // CHECK: Error: Bad alignment
  // CHECK: Error: Bad alignment
  // CHECK: Ok
  // CHECK: Ok
  // CHECK: Ok
  // CHECK: [Trace] Free heap 0x{{.*}}
  performTypeChecks<short>(n, "short");

  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [42 x int]
  // CHECK: Error: Unknown address
  // CHECK: Ok
  // CHECK: Ok
  // CHECK: Ok
  // CHECK: Error: Unknown address
  // CHECK: Error: Bad alignment
  // CHECK: Error: Bad alignment
  // CHECK: Error: Bad alignment
  // CHECK: Ok
  // CHECK: Ok
  // CHECK: [Trace] Free heap 0x{{.*}}
  performTypeChecks<int>(n, "int");

  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [42 x long int]
  // CHECK: Error: Unknown address
  // CHECK: Ok
  // CHECK: Ok
  // CHECK: Ok
  // CHECK: Error: Unknown address
  // CHECK: Error: Bad alignment
  // CHECK: Error: Bad alignment
  // CHECK: Error: Bad alignment
  // CHECK: Error: Bad alignment
  // CHECK: Ok
  // CHECK: [Trace] Free heap 0x{{.*}}
  performTypeChecks<long>(n, "long int");

  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [42 x float]
  // CHECK: Error: Unknown address
  // CHECK: Ok
  // CHECK: Ok
  // CHECK: Ok
  // CHECK: Error: Unknown address
  // CHECK: Error: Bad alignment
  // CHECK: Error: Bad alignment
  // CHECK: Error: Bad alignment
  // CHECK: Ok
  // CHECK: Ok
  // CHECK: [Trace] Free heap 0x{{.*}}
  performTypeChecks<float>(n, "float");

  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [42 x double]
  // CHECK: Error: Unknown address
  // CHECK: Ok
  // CHECK: Ok
  // CHECK: Ok
  // CHECK: Error: Unknown address
  // CHECK: Error: Bad alignment
  // CHECK: Error: Bad alignment
  // CHECK: Error: Bad alignment
  // CHECK: Error: Bad alignment
  // CHECK: Ok
  // CHECK: [Trace] Free heap 0x{{.*}}
  performTypeChecks<double>(n, "double");

  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [42 x int*]
  // CHECK: Error: Unknown address
  // CHECK: Ok
  // CHECK: Ok
  // CHECK: Ok
  // CHECK: Error: Unknown address
  // CHECK: Error: Bad alignment
  // CHECK: Error: Bad alignment
  // CHECK: Error: Bad alignment
  // CHECK: Error: Bad alignment
  // CHECK: Ok
  // CHECK: [Trace] Free heap 0x{{.*}}
  performTypeChecks<int*>(n, "int*");

  return 0;
}
