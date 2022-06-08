// clang-format off
// RUN: %run %s -o -O1 --thread > %s.tsan.log 2>&1
// RUN: %filecheck %s --check-prefix=CHECK-TSAN < %s.tsan.log
// RUN: %run %s -o -O1 --thread > %s.log 2>&1 
// RUN: %filecheck %s < %s.log
// REQUIRES: thread && softcounter
// REQUIRES: tracker

// clang-format on

#include "util.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <random>
#include <thread>
#include <vector>

using namespace typeart;

std::atomic_bool stop{false};
const size_t extent{1};

template <typename S, typename E>
void repeat_alloc(alloc_id_t alloc_id, S s, E e) {
  std::for_each(
      s, e, [&](auto elem) { typeart_tracker_alloc(reinterpret_cast<const void*>(elem), alloc_id.value(), extent); });
}

template <typename S, typename E>
void repeat_alloc_free_v2(alloc_id_t alloc_id, S s, E e) {
  using namespace std::chrono_literals;
  std::for_each(s, e, [&](auto elem) {
    typeart_tracker_alloc(reinterpret_cast<const void*>(elem), alloc_id.value(), extent);
    // std::this_thread::sleep_for(1ms);
    typeart_tracker_free(reinterpret_cast<const void*>(elem));
  });
}

template <typename S, typename E>
void repeat_type_check(alloc_id_t alloc_id, S s, E e) {
  do {
    std::for_each(s, e, [&](auto addr) {
      auto pointer_info_result = PointerInfo::get(reinterpret_cast<const void*>(addr));
      if (pointer_info_result.has_error()) {
        fprintf(stderr, "[Error]: Lookup failed for %i (%#02x)\n", addr, addr);
      } else {
        auto pointer_info = pointer_info_result.value();
        if (pointer_info.getCount() != extent) {
          fprintf(stderr, "[Error]: Length mismatch of %i (%#02x) is: count=%zu\n", addr, addr,
                  pointer_info.getCount());
        }
        if (auto basic_type = meta::dyn_cast<meta::di::BasicType>(&pointer_info.getType());
            basic_type == nullptr || basic_type->get_encoding() != meta::di::Encoding::Float ||
            basic_type->get_size_in_bits() != 64) {
          fprintf(stderr, "[Error]: Type mismatch of %i (%#02x) is: type=%s\n", addr, addr,
                  pointer_info.getType().get_pretty_name().c_str());
        }
      }
    });
  } while (!stop);
}

std::vector<int> unique_rand(const unsigned size) {
  std::vector<int> vec(size);
  unsigned cnt{1};
  std::generate(vec.begin(), vec.end(), [&cnt]() {
    auto current = cnt;
    cnt += extent * sizeof(double);
    return current;
  });
  std::mt19937 g(42);
  std::shuffle(vec.begin(), vec.end(), g);
  return vec;
}

int main(int argc, char** argv) {
  constexpr unsigned size = 100;
  auto vec                = unique_rand(size);
  auto beg                = std::begin(vec);
  auto h                  = beg + (size / 2);
  auto e                  = std::end(vec);

  auto alloc_id = create_fake_double_heap_alloc_id();
  // TODO: this thread in itself has a race condition: if the malloc_1 threads runs slower than
  // the check_1 thread, we get a falied type check
  std::thread malloc_1(repeat_alloc<decltype(beg), decltype(h)>, alloc_id, beg, h);
  std::thread malloc_2(repeat_alloc_free_v2<decltype(h), decltype(e)>, alloc_id, h, e);
  std::thread check_1(repeat_type_check<decltype(beg), decltype(e)>, alloc_id, beg, h);

  malloc_1.join();
  malloc_2.join();

  stop = true;

  check_1.join();

  // CHECK-TSAN-NOT: ThreadSanitizer
  // CHECK-NOT: Error
  return 0;
}
