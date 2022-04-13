#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <dlfcn.h>
#include <runtime/allocator/Allocator.h>
#include <runtime/allocator/Config.h>

extern "C" {
void* typeart_preload_thread_start(void*, void*);
}

namespace typeart::preload {

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
  if (!runtime::allocator::free(ptr)) {
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
  // TODO
}

void* typeart_preload_allocate_stack(void** stack_ptr) {
  using namespace typeart;
  auto current_thread  = pthread_self();
  auto new_stack_begin = runtime::allocator::stack::allocate(current_thread);

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
  auto config_stack_size = runtime::allocator::config::stack::stack_size;
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
  runtime::allocator::stack::free(pthread_self());
}

}  // extern "C"

__asm__(
    // %rdi is the function to call
    // %rsi is the arg for the thread function
    "\t.align 16, 0x90\n"
    "\t.type typeart_preload_thread_start,@function\n"
    "typeart_preload_thread_start:\n"

    // Handle the frame pointer and reserve stack space:
    "\tpushq %rbp\n"
    "\tmovq %rsp, %rbp\n"  // save the current stack pointer to %rbp (BEFORE we reserve stack space!)
    "\tsubq $32, %rsp\n"

    // Save the function arguments on the stack:
    "\tmovq %rdi, -16(%rbp)\n"
    "\tmovq %rsi, -8(%rbp)\n"

    // Allocate the instrumented stack for the thread:
    "\tmovq %rsp, %rdi\n"       // pass the original stack pointer as the first argument
    "\tmovq %rsp, -24(%rbp)\n"  // and also save it on the stack
    "\tmovabsq $typeart_preload_allocate_stack, %rax\n"
    "\tcallq *%rax\n"
    "\tmovq %rax, %rsp\n"

    // Call the actual thread routine:
    "\tmovabsq $typeart_preload_exec_thread, %rax\n"
    "\tmovq -16(%rbp), %rdi\n"
    "\tmovq -8(%rbp), %rsi\n"
    "\tcallq *%rax\n"

    // Restore the original stack pointer and free the stack.
    "\tmovq -24(%rbp), %rsp\n"  // restore the original stack from %rbp
    "\tmovq %rax, %rbp\n"       // save the result in %rbp
    "\tmovabsq $typeart_preload_free_stack, %rax\n"
    "\tcallq *%rax\n"

    // Restore the result and the frame pointer
    "\tmovq %rbp, %rax\n"
    "\taddq $32, %rsp\n"
    "\tpopq %rbp\n"
    "\tretq\n");
