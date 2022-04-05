#include "Allocator.h"

extern "C" {

void* typeart_allocator_malloc(int allocation_id, size_t count, size_t size) {
  return typeart::allocator::malloc(allocation_id, count, size);
}

void* typeart_allocator__Znwm(int allocation_id, size_t count, size_t size) {
  auto result = typeart::allocator::malloc(allocation_id, count, size);
  if (result == nullptr) {
    throw std::bad_alloc{};
  }
  return result;
}

void* typeart_allocator__Znam(int allocation_id, size_t count, size_t size) {
  auto result = typeart::allocator::malloc(allocation_id, count, size);
  if (result == nullptr) {
    throw std::bad_alloc{};
  }
  return result;
}

int typeart_allocator_free(void* addr) {
  return typeart::allocator::free(addr);
}

}  // extern "C"
