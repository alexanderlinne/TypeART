// clang-format off
// RUN: %run %s --thread > %s.log 2>&1
// RUN: cat %s.log | %filecheck %s
// RUN: %run %s --thread 2>&1 | %filecheck %s --check-prefix=CHECK-TSAN
// REQUIRES: thread && softcounter
// REQUIRES: tracker
// clang-format on

#include "util.hpp"

#include <atomic>
#include <pthread.h>
#include <stdlib.h>
#include <thread>

void repeat_alloc_free(unsigned n) {
  for (int i = 0; i < n; i++) {
    double* d = (double*)malloc(sizeof(double) * n);
    free(d);
  }
}

std::atomic_bool stop{false};

void repeat_type_check(float* ptr, int count) {
  do {
    check(ptr, "float", count, 0);
  } while (!stop);
}

int main(int argc, char** argv) {
  constexpr unsigned n = 1000;

  // CHECK: [Trace] TypeART Runtime Trace

  float* f1 = (float*)malloc(sizeof(float) * 10);
  float* f2 = (float*)malloc(sizeof(float) * 20);

  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [10 x float]
  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [20 x float]

  std::thread type_check_1(repeat_type_check, f1, 10);
  std::thread type_check_2(repeat_type_check, f2, 20);

  std::thread malloc_1(repeat_alloc_free, n);
  std::thread malloc_2(repeat_alloc_free, n);
  std::thread malloc_3(repeat_alloc_free, n);

  malloc_1.join();
  malloc_2.join();
  malloc_3.join();

  stop = true;

  type_check_1.join();
  type_check_2.join();

  free(f1);
  free(f2);

  // CHECK-TSAN-NOT: ThreadSanitizer

  // CHECK: Allocation type detail (heap, stack, global)
  // CHECK: double : 3000 ,    0 ,    0
  // CHECK: Free allocation type detail (heap, stack)
  // CHECK: double : 3000 ,    0

  return 0;
}
