#define _GNU_SOURCE

#include <dlfcn.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <threads.h>
#include <unistd.h>

static void* typeart_find_next_symbol(const char* function_name) {
  void* function = dlsym(RTLD_NEXT, function_name);
  if (!function) {
    fprintf(stderr, "Cannot find symbol \'%s\'!\n", function_name);
    abort();
  }
  return function;
}

int typeart_allocator_free(void* addr);
static void (*actual_free)(void*) = NULL;
void free(void* ptr) {
  if (!actual_free) {
    actual_free = typeart_find_next_symbol("free");
  }
  if (!typeart_allocator_free(ptr)) {
    actual_free(ptr);
  }
}
