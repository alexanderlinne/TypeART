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

static pthread_barrier_t barrier;

void* f(void*) {
  if (!stack::is_owner(pthread_self())) {
    fprintf(stderr, "[Error] Thread owns no stack!\n");
  }

  char c;
  if (!stack::is_instrumented(&c)) {
    fprintf(stderr, "[Error] Stack was not replaced!\n");
  }

  pthread_barrier_wait(&barrier);

  pthread_cond_t cv    = PTHREAD_COND_INITIALIZER;
  pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_lock(&lock);
  while (true) {
    pthread_cond_wait(&cv, &lock);
  }
}

int main(int argc, char** argv) {
  pthread_barrier_init(&barrier, NULL, 2);

  constexpr unsigned n = 4;

  // CHECK: [Trace] TypeART Runtime Trace

  // CHECK-NOT: [Error]
  pthread_t thread;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_create(&thread, &attr, f, nullptr);
  pthread_barrier_wait(&barrier);
  pthread_cancel(thread);
  pthread_join(thread, nullptr);
  if (stack::is_owner(thread)) {
    fprintf(stderr, "[Error] Thread stack was not freed!\n");
  }
  pthread_barrier_destroy(&barrier);

  return 0;
}
