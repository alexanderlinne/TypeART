// clang-format off
// RUN: %run %s --thread --compile_flags "-std=c++17" 2>&1 | %filecheck %s
// REQUIRES: thread
// REQUIRES: allocator
// clang-format on

#include "../tracker/util.h"
#include "allocator/Allocator.h"

#include <cassert>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

using namespace typeart::allocator;

void* f(void*) {
  if (!stack::is_owner(pthread_self())) {
    fprintf(stderr, "[Error] Thread owns no stack!\n");
  }

  char c;
  if (!stack::is_instrumented(&c)) {
    fprintf(stderr, "[Error] Stack was not replaced!\n");
  }

  pthread_cond_t cv    = PTHREAD_COND_INITIALIZER;
  pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_lock(&lock);
  while (true) {
    pthread_cond_wait(&cv, &lock);
  }
}

int main(int argc, char** argv) {
  constexpr unsigned n = 4;

  // CHECK: [Trace] TypeART Runtime Trace

  // CHECK-NOT: [Error]
  pthread_t thread;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_create(&thread, &attr, f, nullptr);
  pthread_cancel(thread);
  pthread_join(thread, nullptr);
  if (stack::is_owner(thread)) {
    fprintf(stderr, "[Error] Thread stack was not freed!\n");
  }

  return 0;
}
