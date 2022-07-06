// clang-format off
// RUN: %run %s --thread 2>&1 | %filecheck %s
// REQUIRES: thread
// REQUIRES: allocator
// clang-format on

#include "../tracker/util.hpp"

#include <cassert>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

namespace typeart::allocator::stack {
bool is_owner(pthread_t thread);
bool is_instrumented(void* addr);
}  // namespace typeart::allocator::stack

using namespace typeart::allocator;

void* f(void*) {
  if (!stack::is_owner(pthread_self())) {
    fprintf(stderr, "[Error] Thread owns no stack!\n");
  }

  char c[7];
  double d = 5;
  check(c, "char[7]", 1, 0);
  check(&d, "double", 1, 0);

  if (!stack::is_instrumented(c)) {
    fprintf(stderr, "[Error] Stack was not replaced!\n");
  }

  return nullptr;
}

int main(int argc, char** argv) {
  constexpr unsigned n = 4;

  // CHECK: [Trace] TypeART Runtime Trace

  // CHECK-NOT: [Error]
  // CHECK: Ok
  // CHECK-NEXT: Ok
  // CHECK-NOT: [Error]
  pthread_t thread;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_create(&thread, &attr, f, nullptr);
  pthread_join(thread, nullptr);
  if (stack::is_owner(thread)) {
    fprintf(stderr, "[Error] Thread stack was not freed!\n");
  }

  return 0;
}
