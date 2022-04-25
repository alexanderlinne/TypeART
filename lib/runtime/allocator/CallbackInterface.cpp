#include "CallbackInterface.h"

#include "runtime/allocator/Allocator.hpp"

#include <cstring>

using namespace typeart;
using namespace typeart::runtime;

extern "C" {

void* typeart_allocator_malloc(alloc_id_t::value_type alloc_id, size_t count, size_t size) {
  return allocator::malloc(alloc_id, count, size);
}

void* typeart_allocator_realloc(alloc_id_value alloc_id, size_t count, void* ptr, size_t new_size) {
  return allocator::realloc(alloc_id, count, ptr, new_size);
}

void* typeart_allocator_calloc(alloc_id_t::value_type alloc_id, size_t count, size_t num, size_t size) {
  auto result = allocator::malloc(alloc_id, count, num * size);
  memset(result, 0, num * size);
  return result;
}

void* typeart_allocator__Znwm(alloc_id_t::value_type alloc_id, size_t count, size_t size) {
  auto result = allocator::malloc(alloc_id, count, size);
  if (result == nullptr) {
    throw std::bad_alloc{};
  }
  return result;
}

void* typeart_allocator__Znam(alloc_id_t::value_type alloc_id, size_t count, size_t size) {
  auto result = allocator::malloc(alloc_id, count, size);
  if (result == nullptr) {
    throw std::bad_alloc{};
  }
  return result;
}

int typeart_allocator_free(void* addr) {
  return allocator::free(addr);
}

}  // extern "C"
