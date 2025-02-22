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
void repeat_alloc(meta::meta_id_t meta_id, S s, E e) {
  std::for_each(s, e,
                [&](auto elem) { typeart_tracker_alloc(reinterpret_cast<const void*>(elem), meta_id.value(), 20); });
}

template <typename S, typename E>
void repeat_dealloc(S s, E e) {
  std::for_each(s, e, [&](auto elem) { typeart_tracker_free(reinterpret_cast<const void*>(elem)); });
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
  constexpr unsigned size = 300;
  auto vec                = unique_rand(size);
  auto beg                = std::begin(vec);
  auto h1                 = beg + (size / 3);
  auto h2                 = h1 + (size / 3);
  auto e                  = std::end(vec);

  auto meta_id = create_fake_double_heap_allocation();

#pragma omp parallel sections num_threads(3)
  {
#pragma omp section
    { repeat_alloc(meta_id, beg, h1); }
#pragma omp section
    { repeat_alloc(meta_id, h2, e); }
#pragma omp section
    { repeat_alloc(meta_id, h1, h2); }
  }

#pragma omp parallel sections num_threads(3)
  {
#pragma omp section
    { repeat_dealloc(beg, h1); }
#pragma omp section
    { repeat_dealloc(h2, e); }
#pragma omp section
    { repeat_dealloc(h1, h2); }
  }

  // CHECK-TSAN-NOT: ThreadSanitizer

  // CHECK-NOT: Error

  // CHECK: Allocation type detail (heap, stack, global)
  // CHECK: double : 301 ,     0 ,    0

  // CHECK: Free allocation type detail (heap, stack)
  // CHECK: double : 301 ,     0
  return 0;
}
