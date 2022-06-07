// clang-format off
// RUN: OMP_NUM_THREADS=3 %run %s -o -O1 --omp 2>&1 | %filecheck %s --check-prefix=CHECK-TSAN
// RUN: OMP_NUM_THREADS=3 %run %s -o -O1 --omp 2>&1 | %filecheck %s
// REQUIRES: openmp && softcounter
// REQUIRES: tracker

// clang-format on

#include "util.hpp"

#include <algorithm>
#include <random>
#include <vector>

using namespace typeart;

template <typename S, typename E>
void repeat_alloc(alloc_id_t alloc_id, S s, E e) {
  std::for_each(s, e,
                [&](auto elem) { typeart_tracker_alloc(reinterpret_cast<const void*>(elem), alloc_id.value(), 20); });
}

std::vector<int> unique_rand(const unsigned size) {
  std::vector<int> vec(size);
  std::iota(vec.begin(), vec.end(), 1);

  std::random_device rd;
  std::mt19937 g(42);

  std::shuffle(vec.begin(), vec.end(), g);

  return vec;
}

int main(int argc, char** argv) {
  constexpr unsigned size = 100;
  auto vec                = unique_rand(size);
  auto beg                = std::begin(vec);
  auto e                  = std::end(vec);

  auto alloc_id = create_fake_double_heap_alloc_id();

#pragma omp parallel sections num_threads(3)
  {
#pragma omp section
    { repeat_alloc(alloc_id, beg, e); }
#pragma omp section
    { repeat_alloc(alloc_id, beg, e); }
#pragma omp section
    { repeat_alloc(alloc_id, beg, e); }
  }

  // CHECK-TSAN-NOT: ThreadSanitizer

  // CHECK-NOT: Error

  // 3 Threads, using the same 100 pointers, i.e., 200 are overridden:
  // CHECK: Alloc Stats from softcounters
  // CHECK: Addresses re-used          :  200

  // CHECK: Allocation type detail (heap, stack, global)
  // CHECK: double : 301 ,     0 ,    0
  return 0;
}
