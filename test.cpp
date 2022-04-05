#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

thread_local void* stack_ptr;
thread_local size_t stack_size;

extern "C" {

char** typeart_envp     = nullptr;
void* virtual_stack_ptr = nullptr;

__attribute__((noinline)) void* create_stack(char** stack_begin) {
  auto stack_memory =
      mmap64(nullptr, 1UL << 24, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
  virtual_stack_ptr = stack_memory;

  // From the implementation of _start in glibc we know that the first element
  // at the top of the stack is the terminal nullptr in the envp array. As the
  // _start function does not return, we can safely copy the stack up to this
  // point.
  size_t envp_it = 0;
  while (typeart_envp[envp_it] != nullptr)
    ++envp_it;
  void* stack_end = (void**)&typeart_envp[envp_it + 1];

  // Copy the actual stack data.
  auto current_stack_size = (uintptr_t)stack_end - (uintptr_t)stack_begin;
  assert(*((void**)((char*)stack_begin + current_stack_size) - 1) == nullptr);
  auto new_stack_end   = (void**)((char*)stack_memory + (1UL << 24));
  auto new_stack_begin = (void**)((char*)new_stack_end - current_stack_size);
  memcpy(new_stack_begin, stack_begin, current_stack_size);

  // Replace any values on the new stack that look like pointers to the old
  // stack. This is necessary as stack pointers may have been stored on the
  // stack that may later be recovered leading to the program using the original
  // stack again. Just brute-force replacing those values is unsafe as
  // unrelated values may be overwritten, but this is extremely unlikely.
  const auto offset = (intptr_t)new_stack_begin - (intptr_t)stack_begin;
  for (auto stack_it = new_stack_end - 1; stack_it >= new_stack_begin; --stack_it) {
    if (stack_begin <= *stack_it && *stack_it < stack_end) {
      *stack_it = (char*)*stack_it + offset;
    }
  }

  return new_stack_begin;
}

void stack_pivot();
}

__asm__(
    "\t.align 16, 0x90\n"
    "\t.type stack_pivot,@function\n"
    "stack_pivot:\n"
    "\tmovq %rsp, %rdi\n"
    "\tmovabsq $create_stack, %rax\n"
    "\tcallq *%rax\n"
    "\tmovq %rax, %rsp\n"
    "\tretq\n");

void __test_preinit(int argc, char** argv, char** envp) {
  typeart_envp = envp;
  stack_pivot();
}

__attribute__((section(".preinit_array"), used)) void (*__test_preinit_arr)(int, char**, char**) = __test_preinit;

struct alignas(64) test {};

extern char __executable_start, etext, edata;

void init_stack_attr() {
  auto this_thread = pthread_self();
  pthread_attr_t attr;
  if (pthread_getattr_np(this_thread, &attr) != 0) {
    abort();
  }
  if (pthread_attr_getstack(&attr, &stack_ptr, &stack_size) != 0) {
    abort();
  }
}

class file_descriptor {
  int fd;

 public:
  file_descriptor(int fd) : fd(fd) {
  }
  ~file_descriptor() {
    if (close(fd) != 0) {
      abort();
    }
  }

  operator int() const {
    return fd;
  }
};

file_descriptor get_stack_fd() {
  auto path   = "/proc/self/mem";
  auto result = open(path, O_RDWR);
  if (result == -1) {
    printf("could not open file\n");
    abort();
  }
  return result;
}

thread_local const file_descriptor stack_fd = get_stack_fd();

int main(int argc, char** argv, char** envp) {
  printf("%p %p\n", virtual_stack_ptr, (char*)virtual_stack_ptr + (1UL << 24));
  init_stack_attr();
  printf("%d, %d\n", argc, *(int*)(argv - 1));
  char anchor = 'A';
  auto env_it = envp;
  for (; *env_it != nullptr; ++env_it) {
    // printf("%p\t", env_it);
    // printf("%p\t", *env_it);
    // for (auto it = *env_it; *it != '\0'; ++it) {
    //   printf("%c", *it);
    // }
    // printf("\n");
  }
  auto stack_end   = env_it;
  auto pthread_end = (char**)((char*)stack_ptr + stack_size);
  printf("%p, %p, %p\n", &anchor, pthread_end, stack_end);
  // for (auto it = stack_end; it >= (char**)&anchor; it--) {
  //   // if (*it == '\0') {
  //   //   printf("\n");
  //   // } else {
  //   printf("%p %d %p\n", it, *it >= stack_ptr && *it < (char*)pthread_end, *it);
  //   //    }
  // }
  printf("\nSTACK_END %ld\n", stack_size);

  // printf("%ld %ld\n", alignof(test), alignof(std::max_align_t));

  // printf("%ld\n", stack_size);

  // printf("%p, %p, %p\n", &__executable_start, &etext, &edata);
  // auto fp = fopen("/proc/self/maps", "r");
  // if (fp == NULL) {
  //   perror("Error opening file");
  //   return 0;
  // }
  // char line[2048];
  // while (fgets(line, 2048, fp) != NULL) {
  //   printf("%s", line);
  // }
  // printf("%ld\n", sysconf(_SC_PAGESIZE));

  // int i = 42;

  // auto i_off = (char*)&i - (char*)stack_ptr;
  // printf("%p, %p, %lx\n", stack_ptr, &i, i_off);

  // lseek(stack_fd, (intptr_t)stack_ptr + i_off, SEEK_SET);
  // int test;
  // read(stack_fd, &test, sizeof(int));
  // printf("read %d\n", test);
  // lseek(stack_fd, 0, SEEK_SET);

  // auto mapped = mmap(nullptr, stack_size, PROT_READ | PROT_WRITE,
  //                    MAP_SHARED_VALIDATE | MAP_STACK | MAP_NORESERVE | MAP_GROWSDOWN, stack_fd, (intptr_t)stack_ptr);
  // if (mapped == MAP_FAILED) {
  //   printf("%d\n", errno);
  //   abort();
  // }
  // printf("%p\n", mapped);

  // auto i_mapped = (int*)((char*)mapped + i_off);
  // printf("%d, %d\n", i, *i_mapped);
}
