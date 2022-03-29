#include <algorithm>
#include <cstddef>
#include <unistd.h>

namespace typeart::allocator::config {

constexpr size_t page_size = 4096;

namespace heap {

constexpr size_t region_size         = 1UL << 32;  // 4GB
constexpr size_t min_allocation_size = 1UL << 5;   // 32B
constexpr size_t max_allocation_size = 1UL << 30;  // 1GB

constexpr size_t region_count  = __builtin_clzll(min_allocation_size) - __builtin_clzll(max_allocation_size) + 1;
constexpr size_t regions_begin = 64 - __builtin_clzll(min_allocation_size);
constexpr size_t regions_end   = regions_begin + region_count;
constexpr size_t memory_size   = region_count * region_size;

// Assert that the region size and allocation sizes are powers of two
static_assert(__builtin_popcountll(min_allocation_size) == 1);
static_assert(__builtin_popcountll(max_allocation_size) == 1);
static_assert(__builtin_popcountll(region_size) == 1);

// All dynamically allocated memory must be properly aligned for all types.
// Thus an allocation size smaller than the maximum alignment is not sensible.
static_assert(min_allocation_size >= alignof(std::max_align_t));

static_assert(region_size >= max_allocation_size);
static_assert(region_size % page_size == 0);
static_assert(max_allocation_size > min_allocation_size);
static_assert(max_allocation_size >= page_size && max_allocation_size % page_size == 0);

// The amout of bytes needed for TypeART data.
// Should be a power of two for easier calculations.
constexpr size_t required_offset = 16;
constexpr size_t max_align       = alignof(std::max_align_t);
static_assert(__builtin_popcountll(required_offset) == 1);
static_assert(__builtin_popcountll(max_align) == 1);
constexpr size_t allocation_offset = std::max(required_offset, max_align);

// An allocation size which can only hold the TypeART data would not be sensible.
static_assert(min_allocation_size > required_offset);

}  // namespace heap

namespace stack {

constexpr size_t thread_count        = 16;
constexpr size_t stack_size          = 1UL << 23;  // 8MB
constexpr size_t region_size         = thread_count * stack_size;
constexpr size_t guard_size          = 2 * page_size;
constexpr size_t guarded_region_size = region_size + guard_size;
constexpr size_t min_allocation_size = 1UL << 5;         // 32B
constexpr size_t max_allocation_size = stack_size >> 1;  // 4MB

constexpr size_t region_count  = __builtin_clzll(min_allocation_size) - __builtin_clzll(max_allocation_size) + 1;
constexpr size_t regions_begin = 64 - __builtin_clzll(min_allocation_size);
constexpr size_t regions_end   = regions_begin + region_count;

// Assert that the region size and allocation sizes are powers of two
static_assert(__builtin_popcountll(min_allocation_size) == 1);
static_assert(__builtin_popcountll(max_allocation_size) == 1);
static_assert(__builtin_popcountll(stack_size) == 1);
static_assert(__builtin_popcountll(region_size) == 1);

constexpr size_t LLVM_max_alignment = 1UL << 32;
static_assert(stack_size >= max_allocation_size);
static_assert(max_allocation_size > min_allocation_size);
static_assert(max_allocation_size <= LLVM_max_alignment);
static_assert(region_size % page_size == 0);

constexpr size_t index_for(size_t size) {
  auto region_idx = 64 - (size_t)__builtin_clzll(size) + (__builtin_popcountll(size) == 1 ? 0 : 1);
  if (region_idx < regions_begin) {
    return 0;
  } else {
    return region_idx - regions_begin;
  }
}

constexpr size_t next_power_of_two(size_t n) {
  if (__builtin_popcountll(n) == 1) {
    return n;
  } else {
    return 1UL << (64 - __builtin_clzll(n));
  }
}

constexpr size_t alignment_for(size_t size) {
  return next_power_of_two(size);
}

constexpr size_t region_offset_for(size_t size) {
  return (index_for(size) + 1) * guarded_region_size;
}

}  // namespace stack

}  // namespace typeart::allocator::config
