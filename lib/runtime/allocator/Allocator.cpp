#include "Allocator.h"

#include "../Runtime.h"
#include "Config.h"

#include <fmt/core.h>
#include <fmt/ostream.h>
#include <mutex>
#include <set>
#include <sys/mman.h>

// We require to be on a 64bit architecture
static_assert(sizeof(void*) == sizeof(std::int64_t));

namespace typeart::runtime::allocator {

void* reserve_virtual_memory(size_t size) {
  return mmap64(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
}

void* reserve_virtual_memory(size_t size, int fd) {
  return mmap64(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, fd, 0);
}

void* remap_virtual_memory(void* addr, size_t size, int fd) {
  return mmap64(addr, size, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, fd, 0);
}

namespace heap {

constexpr size_t region_size         = config::heap::region_size;
constexpr size_t region_count        = config::heap::region_count;
constexpr size_t min_allocation_size = config::heap::min_allocation_size;
constexpr size_t max_allocation_size = config::heap::max_allocation_size;
constexpr size_t regions_begin       = config::heap::regions_begin;
constexpr size_t regions_end         = config::heap::regions_end;
constexpr size_t min_alignment       = config::heap::min_alignment;

constexpr size_t virtual_memory_size = region_count * region_size + max_allocation_size;

struct Region {
  void* begin;
  void* free_begin;
  void* end;
  size_t allocation_size;
  std::set<void*> free_list;
  std::mutex mutex;

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
    free_begin        = (int8_t*)free_begin + allocation_size;
    if (free_begin >= end) {
      return nullptr;
    }
    return result;
  }

  void free(void* addr) {
    addr = (int8_t*)addr - min_alignment;
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

  std::optional<PointerInfo> getPointerInfo(const void* addr) {
    if (addr >= begin && addr < end) {
      auto bucket_ptr      = (void*)((uintptr_t)addr & ~(allocation_size - 1));
      auto alloc_id        = *(alloc_id_t*)bucket_ptr;
      auto allocation_info = Runtime::getAllocationInfo(alloc_id);
      if (allocation_info == nullptr) {
        fmt::print(stderr, "Found invalid allocaton_id {}!\n", alloc_id);
        return {};
      }
      auto base_ptr = (void*)((int8_t*)bucket_ptr + allocation_info->base_ptr_offset.value_or(heap::min_alignment));
      auto count    = *(size_t*)((int8_t*)bucket_ptr + config::count_offset);
      return PointerInfo{base_ptr, alloc_id, count, nullptr};
    } else {
      return {};
    }
  }
};

void* begin = nullptr;
void* end   = nullptr;
static Region regions[region_count];

bool initialized = false;

__attribute__((constructor)) void ctor() {
  begin = reserve_virtual_memory(virtual_memory_size);
  end   = (int8_t*)begin + virtual_memory_size;

  // We potentially need to offset the pointer such that it is properly aligned
  // for the max_allocation_size region.
  void* regions_ptr = begin;
  if (((uintptr_t)regions_ptr & (max_allocation_size - 1)) != 0) {
    regions_ptr = (int8_t*)((uintptr_t)regions_ptr & ~(max_allocation_size - 1)) + max_allocation_size;
  }

  for (auto i = size_t{0}; i < region_count; i++) {
    auto region_begin = (int8_t*)regions_ptr + i * region_size;
    regions[i].initialize(region_begin, region_begin + region_size, min_allocation_size << i);
  }
  initialized = true;
}

__attribute__((destructor)) void dtor() {
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

size_t index_for(const void* addr) {
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

Region* region_for(const void* addr) {
  return &regions[index_for(addr)];
}

std::optional<PointerInfo> getPointerInfo(const void* addr) {
  if (addr >= begin && addr < end) {
    return region_for(addr)->getPointerInfo(addr);
  }
  return {};
}

}  // namespace heap

void* malloc(alloc_id_t alloc_id, size_t count, size_t size) {
  if (!heap::initialized) {
    return ::malloc(size);
  }
  auto required_size = size + heap::min_alignment;
  auto region        = heap::region_for(required_size);
  if (!region) {
    return ::malloc(size);
  }
  assert(required_size <= region->allocation_size);
  auto allocation                                        = (size_t*)region->allocate();
  *(alloc_id_t*)allocation                               = alloc_id;
  *(size_t*)((int8_t*)allocation + config::count_offset) = count;
  return (int8_t*)allocation + heap::min_alignment;
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

constexpr size_t thread_count        = config::stack::thread_count;
constexpr size_t region_size         = config::stack::region_size;
constexpr size_t region_count        = config::stack::region_count;
constexpr size_t guard_size          = config::stack::guard_size;
constexpr size_t guarded_region_size = config::stack::guarded_region_size;
constexpr size_t stack_size          = config::stack::stack_size;
constexpr size_t min_allocation_size = config::stack::min_allocation_size;

constexpr size_t virtual_memory_size = (region_count + 1) * (region_size + 2 * config::page_size);

int fd             = 0;
void* begin        = nullptr;
void* end          = nullptr;
void* mapped_begin = nullptr;
void* mapped_end   = nullptr;

void* main_end = nullptr;

std::mutex owner_mutex;
void* stack_ptr[thread_count];
bool has_owner[thread_count];
pthread_t owner[thread_count];

void setup() {
  if (begin != nullptr) {
    return;
  }
  begin = reserve_virtual_memory(virtual_memory_size);
  end   = (int8_t*)begin + virtual_memory_size;

  // Set up the main stack memory.
  fd = memfd_create("typeart_stack", MFD_CLOEXEC);
  ftruncate64(fd, region_size);
  remap_virtual_memory(begin, region_size, fd);
  mprotect((int8_t*)begin + region_size, guard_size, PROT_NONE);

  // Setup stack pointers.
  for (size_t i = 0; i < thread_count; ++i) {
    stack_ptr[i] = (int8_t*)begin + i * stack_size;
  }
  // The first stack is owned by the main stack.
  has_owner[0] = true;
  main_end     = (int8_t*)stack_ptr[0] + stack_size;

  // Set up the mapped regions.
  mapped_begin = (int8_t*)begin + guarded_region_size;
  mapped_end   = end;
  for (size_t i = 0; i < region_count; ++i) {
    void* region_begin = (int8_t*)mapped_begin + i * guarded_region_size;
    remap_virtual_memory(region_begin, region_size, fd);
    mprotect((int8_t*)region_begin + region_size, guard_size, PROT_NONE);
  }
}

void* allocate(pthread_t new_owner) {
  if (begin == nullptr) {
    setup();
  }
  std::lock_guard _lock(owner_mutex);
  for (size_t i = 1; i < thread_count; ++i) {
    if (!has_owner[i]) {
      has_owner[i] = true;
      owner[i]     = new_owner;
      return stack_ptr[i];
    }
  }
  abort();
}

bool is_owner(pthread_t thread) {
  for (size_t i = 1; i < thread_count; ++i) {
    if (has_owner[i] && pthread_equal(owner[i], thread)) {
      return true;
    }
  }
  return false;
}

bool is_instrumented(void* addr) {
  return mapped_begin <= addr && addr < mapped_end;
}

void free(pthread_t current_owner) {
  std::lock_guard _lock(owner_mutex);
  for (size_t i = 1; i < thread_count; ++i) {
    if (pthread_equal(owner[i], current_owner)) {
      has_owner[i] = false;
      return;
    }
  }
  fmt::print(stderr, "typeart::allocator::stack::free called with unknown thread id!\n");
  abort();
}

size_t index_for(const void* addr) {
  return ((uintptr_t)addr - (uintptr_t)mapped_begin) / guarded_region_size;
}

size_t allocation_size_for(const void* addr) {
  return min_allocation_size << index_for(addr);
}

std::optional<PointerInfo> getPointerInfo(const void* addr) {
  if (addr >= mapped_begin && addr < mapped_end) {
    const auto allocation_size = allocation_size_for(addr);
    auto bucket_ptr            = (void*)((uintptr_t)addr & ~(allocation_size - 1));
    auto alloc_id              = *(alloc_id_t*)bucket_ptr;
    auto allocation_info       = Runtime::getAllocationInfo(alloc_id);
    if (allocation_info == nullptr) {
      fmt::print(stderr, "Found invalid allocaton_id {}!\n", alloc_id);
      return {};
    }
    if (!allocation_info->base_ptr_offset.has_value()) {
      fmt::print(stderr, "Missing base pointer offset for stack allocation at {} with allocation id {}!\n", addr,
                 alloc_id);
      return {};
    }
    auto base_ptr = (void*)((int8_t*)bucket_ptr + allocation_info->base_ptr_offset.value());
    auto count    = size_t{0};
    if (allocation_info->count.has_value()) {
      count = allocation_info->count.value();
    } else {
      count = *(size_t*)((int8_t*)bucket_ptr + config::count_offset);
    }
    return PointerInfo{base_ptr, alloc_id, count, nullptr};
  } else {
    return {};
  }
}

}  // namespace stack

std::optional<PointerInfo> getPointerInfo(const void* addr) {
  if (auto result = heap::getPointerInfo(addr); result.has_value()) {
    return result;
  }
  if (auto result = stack::getPointerInfo(addr); result.has_value()) {
    return result;
  }
  return Runtime::getTracker().getPointerInfo(addr);
}

}  // namespace typeart::runtime::allocator

using namespace typeart::runtime;

extern "C" {

__attribute__((noinline)) void* typeart_allocator_copy_main_stack(char** envp, void* stack_begin) {
  // From the implementation of _start in glibc we know that the first element
  // at the top of the stack is the terminal nullptr in the envp array. As the
  // _start function does not return, we can safely copy the stack up to this
  // point.
  size_t envp_it = 0;
  while (envp[envp_it] != nullptr)
    ++envp_it;
  void* stack_end = (void**)&envp[envp_it + 1];

  auto current_stack_size = (uintptr_t)stack_end - (uintptr_t)stack_begin;
  auto new_stack_end      = allocator::stack::main_end;
  auto new_stack_begin    = (void*)((int8_t*)new_stack_end - current_stack_size);

  // As stack_begin is the stack pointer taken upon entry to the caller of this
  // function we expect it to be aligned to a 16 byte boundary. To be sure that
  // the replacement does not cause any problems, we must ensure that our new
  // stack begin is also aligned that way.
  auto alignment_mask = 16UL - 1;
  assert(((uintptr_t)stack_begin & alignment_mask) == 0);
  if (((uintptr_t)new_stack_begin & alignment_mask) != 0) {
    auto aligned    = (uintptr_t)new_stack_begin & ~alignment_mask;
    auto offset     = (uintptr_t)new_stack_begin - aligned;
    new_stack_begin = (void*)aligned;
    new_stack_end   = (int8_t*)new_stack_end - offset;
  }

  // Copy the data from the old stack onto the new stack.
  memcpy(new_stack_begin, stack_begin, current_stack_size);

  // Replace any values on the new stack that look like pointers to the old
  // stack. This is necessary as stack pointers may have been stored on the
  // stack that may later be recovered leading to the program using the original
  // stack again. Just brute-force replacing those values is unsafe as
  // unrelated values may be overwritten, but this is extremely unlikely.
  const auto offset = (intptr_t)new_stack_begin - (intptr_t)stack_begin;
  for (auto stack_it = (void**)new_stack_end - 1; stack_it >= new_stack_begin; --stack_it) {
    if (stack_begin <= *stack_it && *stack_it < stack_end) {
      *stack_it = (int8_t*)*stack_it + offset;
    }
  }

  return new_stack_begin;
}

void typeart_allocator_replace_main_stack(char** envp);

void typeart_allocator_setup_main_stack(int argc, char** argv, char** envp) {
  assert(allocator::config::page_size == sysconf(_SC_PAGE_SIZE));
  allocator::stack::setup();
  typeart_allocator_replace_main_stack(envp);
}

}  // extern "C"
