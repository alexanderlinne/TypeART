#include "Allocator.h"

#include "Config.h"

#include <fmt/core.h>
#include <mutex>
#include <set>
#include <sys/mman.h>

// We require to be on a 64bit architecture
static_assert(sizeof(void*) == sizeof(std::int64_t));

namespace typeart::allocator {

void* reserve_virtual_memory(size_t size) {
  return mmap64(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
}

void* reserve_virtual_memory(size_t size, int fd) {
  return mmap64(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, fd, 0);
}

void* remap_virtual_memory(void* addr, size_t size, int fd) {
  return mmap64(addr, size, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, fd, 0);
}

struct Region {
  void* begin;
  void* free_begin;
  void* end;
  size_t allocation_size;
  std::set<void*> free_list;
  std::mutex mutex;

  Region() {
  }

  void initialize(void* new_begin, void* new_end, size_t new_allocation_size) {
    begin           = new_begin;
    free_begin      = new_begin;
    end             = new_end;
    allocation_size = new_allocation_size;
  }

  void* allocate() {
    std::lock_guard _lock(mutex);

    if (!free_list.empty()) {
      const auto result = *free_list.begin();
      free_list.erase(free_list.begin());
      return result;
    }

    const auto result = free_begin;
    free_begin        = (char*)free_begin + allocation_size;
    if (free_begin >= end) {
      return nullptr;
    }
    return result;
  }

  void free(void* addr) {
    addr = (char*)addr - config::heap::allocation_offset;
    std::lock_guard _lock(mutex);
    if (((uintptr_t)addr & (allocation_size - 1)) != 0 || addr >= free_begin) {
      fmt::print(stderr, "TypeART: free on invalid pointer");
      abort();
    }
    if (free_list.find(addr) != free_list.end()) {
      fmt::print(stderr, "TypeART: double free");
      abort();
    }
    free_list.insert(addr);
  }

  std::optional<AllocationInfo> getAllocationInfo(const void* addr) {
    if (addr >= begin && addr < end) {
      auto pointer_info = (PointerInfo*)((uintptr_t)addr & ~(allocation_size - 1));
      return AllocationInfo{PointerInfo{pointer_info->typeId, pointer_info->count, nullptr},
                            (void*)((char*)pointer_info + 16)};
    } else {
      return {};
    }
  }
};

namespace heap {

constexpr size_t region_size         = config::heap::region_size;
constexpr size_t region_count        = config::heap::region_count;
constexpr size_t min_allocation_size = config::heap::min_allocation_size;
constexpr size_t max_allocation_size = config::heap::max_allocation_size;
constexpr size_t regions_begin       = config::heap::regions_begin;
constexpr size_t regions_end         = config::heap::regions_end;

constexpr size_t virtual_memory_size = region_count * region_size + max_allocation_size;

void* begin = nullptr;
void* end   = nullptr;
static Region regions[region_count];

bool initialized = false;

__attribute__((constructor(99999))) void ctor() {
  begin = reserve_virtual_memory(virtual_memory_size);
  end   = (char*)begin + virtual_memory_size;

  // We potentially need to offset the pointer such that it is properly aligned
  // for the max_allocation_size region.
  void* regions_ptr = begin;
  if (((uintptr_t)regions_ptr & (max_allocation_size - 1)) != 0) {
    regions_ptr = (char*)((uintptr_t)regions_ptr & ~(max_allocation_size - 1)) + max_allocation_size;
  }

  for (auto i = size_t{0}; i < region_count; i++) {
    auto region_begin = (char*)regions_ptr + i * region_size;
    regions[i].initialize(region_begin, region_begin + region_size, min_allocation_size << i);
  }
  initialized = true;
}

__attribute__((destructor(99999))) void dtor() {
  initialized = false;
}

constexpr static size_t index_for(size_t size) {
  const auto region_idx = 64 - (size_t)__builtin_clzll(size) + 1;
  if (region_idx < regions_begin) {
    return 0;
  } else {
    return region_idx - regions_begin;
  }
}

size_t index_for(void* addr) {
  return ((uintptr_t)addr - (uintptr_t)begin) / region_size;
}

Region* region_for(size_t size) {
  const auto index = index_for(size);
  if (index >= regions_end) {
    return nullptr;
  } else {
    return &regions[index];
  }
}

Region* region_for(void* addr) {
  return &regions[index_for(addr)];
}

std::optional<AllocationInfo> getAllocationInfo(const void* addr) {
  if (addr >= begin && addr < end) {
    for (auto& region : regions) {
      auto result = region.getAllocationInfo(addr);
      if (result.has_value()) {
        return result;
      }
    }
  }
  return {};
}

}  // namespace heap

void* malloc(int type_id, size_t count, size_t size) {
  if (!heap::initialized) {
    return ::malloc(size);
  }
  auto required_size = size + config::heap::allocation_offset;
  auto region        = heap::region_for(required_size);
  if (!region) {
    return ::malloc(size);
  }
  assert(required_size <= region->allocation_size);
  auto result = (size_t*)region->allocate();
  result[0]   = type_id;
  result[1]   = count;
  auto ret    = (void*)((char*)result + config::heap::allocation_offset);
  return ret;
}

bool free(void* addr) {
  if (!heap::initialized) {
    return heap::begin <= addr && addr < heap::end;
  }
  if (heap::begin <= addr && addr < heap::end) {
    heap::region_for(addr)->free(addr);
    return true;
  } else {
    return false;
  }
}

namespace stack {

constexpr size_t region_size         = config::stack::region_size;
constexpr size_t region_count        = config::stack::region_count;
constexpr size_t guard_size          = config::stack::guard_size;
constexpr size_t guarded_region_size = config::stack::guarded_region_size;
constexpr size_t stack_size          = config::stack::stack_size;
constexpr size_t regions_begin       = config::stack::regions_begin;

constexpr size_t virtual_memory_size = (region_count + 1) * (region_size + 2 * config::page_size);

int fd             = 0;
void* begin        = nullptr;
void* end          = nullptr;
void* mapped_begin = nullptr;
void* mapped_end   = nullptr;

void* main_end = nullptr;

void setup() {
  begin = reserve_virtual_memory(virtual_memory_size);
  end   = (char*)begin + virtual_memory_size;

  // Set up the main stack memory.
  fd = memfd_create("__typeart_stack", MFD_CLOEXEC);
  ftruncate64(fd, region_size);
  remap_virtual_memory(begin, region_size, fd);
  mprotect((char*)begin + region_size, guard_size, PROT_NONE);

  // Set up the mapped regions.
  mapped_begin = (char*)begin + guarded_region_size;
  mapped_end   = end;
  main_end     = (char*)mapped_begin + stack_size;
  for (size_t i = 0; i < region_count; ++i) {
    void* region_begin = (char*)mapped_begin + i * guarded_region_size;
    remap_virtual_memory(region_begin, region_size, fd);
    mprotect((char*)region_begin + region_size, guard_size, PROT_NONE);
  }
}

size_t index_for(const void* addr) {
  return ((uintptr_t)addr - (uintptr_t)mapped_begin) / guarded_region_size - 1;
}

size_t allocation_size_for(const void* addr) {
  return 1UL << (index_for(addr) + regions_begin - 1);
}

std::optional<AllocationInfo> getAllocationInfo(const void* addr) {
  if (addr >= mapped_begin && addr < mapped_end) {
    const auto allocation_size = allocation_size_for(addr);
    auto pointer_info          = (PointerInfo*)((uintptr_t)addr & ~(allocation_size - 1));
    return AllocationInfo{PointerInfo{pointer_info->typeId, pointer_info->count, nullptr},
                          (void*)((char*)pointer_info + 16)};
  } else {
    return {};
  }
}

}  // namespace stack

std::optional<AllocationInfo> getAllocationInfo(const void* addr) {
  const auto result = heap::getAllocationInfo(addr);
  if (result.has_value()) {
    return result;
  }
  return stack::getAllocationInfo(addr);
}

}  // namespace typeart::allocator

extern "C" {

__attribute__((noinline)) void* typeart_copy_main_stack(char** envp, void** stack_begin) {
  // From the implementation of _start in glibc we know that the first element
  // at the top of the stack is the terminal nullptr in the envp array. As the
  // _start function does not return, we can safely copy the stack up to this
  // point.
  size_t envp_it = 0;
  while (envp[envp_it] != nullptr)
    ++envp_it;
  void* stack_end = (void**)&envp[envp_it + 1];

  // Copy the actual stack data.
  auto current_stack_size = (uintptr_t)stack_end - (uintptr_t)stack_begin;
  void** new_stack_end    = (void**)typeart::allocator::stack::main_end;
  void** new_stack_begin  = (void**)((uintptr_t)((char*)new_stack_end - current_stack_size) & ~(8UL - 1));
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

void typeart_replace_main_stack(char** envp);

void typeart_setup_main_stack(int argc, char** argv, char** envp) {
  assert(typeart::allocator::config::page_size == sysconf(_SC_PAGE_SIZE));
  typeart::allocator::stack::setup();
  typeart_replace_main_stack(envp);
}

}  // extern "C"

__asm__(
    "\t.align 16, 0x90\n"
    "\t.type typeart_replace_main_stack,@function\n"
    "typeart_replace_main_stack:\n"
    // %rdi is the first argument (envp)
    "\tmovq %rsp, %rsi\n"  // pass the stack pointer as the second argument
    "\tmovabsq $typeart_copy_main_stack, %rax\n"
    "\tcallq *%rax\n"
    "\tmovq %rax, %rsp\n"  // copy the result of the call into the stack pointer
    "\tretq\n");
