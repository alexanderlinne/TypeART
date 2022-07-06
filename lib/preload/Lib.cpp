#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "runtime/allocator/Allocator.hpp"
#include "runtime/allocator/Config.h"

#include <cstring>
#include <dlfcn.h>

extern "C" {
void* typeart_preload_thread_start(void*, void*);
void typeart_preload_reclaim_capture_stack_ptr();
}

namespace typeart::preload {

static thread_local void* entry_stack_ptr       = nullptr;
static thread_local void* instr_entry_stack_ptr = nullptr;

static void* find_next_symbol(const char* function_name) {
  void* function = dlsym(RTLD_NEXT, function_name);
  if (!function) {
    fprintf(stderr, "Cannot find symbol \'%s\'!\n", function_name);
    abort();
  }
  return function;
}

struct wrapper_args {
  void* (*start_routine)(void*);
  void* arg;
};

void* thread_wrapper(void* _args) {
  auto args = (wrapper_args*)_args;
  return typeart_preload_thread_start((void*)args->start_routine, args->arg);
}

using free_t           = decltype(&free);
using pthread_create_t = decltype(&pthread_create);

static free_t actual_free                     = NULL;
static pthread_create_t actual_pthread_create = NULL;

__attribute__((constructor)) void preload_init() {
  actual_pthread_create = (pthread_create_t)find_next_symbol("pthread_create");
}

}  // namespace typeart::preload

// Preloaded functions
extern "C" {

void free(void* ptr) {
  using namespace typeart;
  if (!preload::actual_free) {
    preload::actual_free = (preload::free_t)preload::find_next_symbol("free");
  }
  if (!allocator::free(ptr)) {
    preload::actual_free(ptr);
  }
}

int pthread_create(pthread_t* thread, const pthread_attr_t* attr, void* (*start_routine)(void*), void* arg) {
  using namespace typeart;
  auto args           = (preload::wrapper_args*)malloc(sizeof(preload::wrapper_args));
  args->start_routine = start_routine;
  args->arg           = arg;
  return preload::actual_pthread_create(thread, attr, preload::thread_wrapper, args);
}
}

// Thread stack replacement
extern "C" {

void typeart_preload_reclaim_stack(void* args) {
  using namespace typeart;
  // calls typeart_preload_reclaim_stack_impl with the current stack ptr
  typeart_preload_reclaim_capture_stack_ptr();
}

void* typeart_preload_reclaim_stack_impl(void* stack_ptr) {
  using namespace typeart;
  auto size          = (uint8_t*)preload::instr_entry_stack_ptr - (uint8_t*)stack_ptr;
  auto new_stack_ptr = (uint8_t*)preload::entry_stack_ptr - size;
  memcpy(new_stack_ptr, stack_ptr, size);
  return new_stack_ptr;
}

void* typeart_preload_allocate_stack(void** stack_ptr) {
  using namespace typeart;
  preload::entry_stack_ptr = stack_ptr;
  auto current_thread      = pthread_self();
  auto new_stack_begin     = allocator::stack::allocate(current_thread);

  pthread_attr_t attr;
  if (pthread_getattr_np(current_thread, &attr) != 0) {
    fprintf(stderr, "pthread_getattr_np failed!\n");
    abort();
  }

  void* stack_begin = nullptr;
  size_t stack_size = 0;
  if (pthread_attr_getstack(&attr, &stack_begin, &stack_size) != 0) {
    fprintf(stderr, "pthread_attr_getstack failed!\n");
    abort();
  }
  auto config_stack_size = allocator::config::stack::stack_size;
  if (stack_size > config_stack_size) {
    fprintf(stderr, "Configured stack size %ld is insufficient for required stack size %ld!\n", config_stack_size,
            stack_size);
  }

  auto stack_end          = (void*)((int8_t*)stack_begin + stack_size);
  auto new_stack_end      = (void*)((int8_t*)new_stack_begin + config_stack_size);
  auto current_stack_size = (uintptr_t)stack_end - (uintptr_t)stack_ptr;
  auto new_stack_ptr      = (void*)((int8_t*)new_stack_end - current_stack_size);
  // We don't have to actuall copy the stack as in the function where we swap the
  // stack pointer we also swap it back.

  preload::instr_entry_stack_ptr = new_stack_ptr;
  return new_stack_ptr;
}

void* typeart_preload_exec_thread(void* (*routine)(void*), void* arg) {
  void* result = nullptr;
  pthread_cleanup_push(&typeart_preload_reclaim_stack, nullptr);
  result = routine(arg);
  pthread_cleanup_pop(0);
  return result;
}

void typeart_preload_free_stack() {
  // Here we don't have to copy the stack back, as
  // our caller should be the exact same caller as
  // the one who called typeart_allocate_stack.
  using namespace typeart;
  allocator::stack::free(pthread_self());
}

}  // extern "C"
