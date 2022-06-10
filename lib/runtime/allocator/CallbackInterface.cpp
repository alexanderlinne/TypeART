#include "CallbackInterface.h"

#include "runtime/allocator/Allocator.hpp"

#include <cstring>

using namespace typeart;

extern "C" {

void* typeart_allocator_malloc(meta_id_t::value_type meta_id, size_t count, size_t size) {
  return allocator::malloc(meta_id, count, size);
}

void* typeart_allocator_realloc(meta_id_value meta_id, size_t count, void* ptr, size_t new_size) {
  return allocator::realloc(meta_id, count, ptr, new_size);
}

void* typeart_allocator_calloc(meta_id_t::value_type meta_id, size_t count, size_t num, size_t size) {
  auto result = allocator::malloc(meta_id, count, num * size);
  if (result != nullptr) {
    memset(result, 0, num * size);
  }
  return result;
}

void* typeart_allocator__Znwm(meta_id_t::value_type meta_id, size_t count, size_t size) {
  auto result = allocator::malloc(meta_id, count, size);
  if (result == nullptr) {
    throw std::bad_alloc{};
  }
  return result;
}

void* typeart_allocator__Znam(meta_id_t::value_type meta_id, size_t count, size_t size) {
  auto result = allocator::malloc(meta_id, count, size);
  if (result == nullptr) {
    throw std::bad_alloc{};
  }
  return result;
}

int typeart_allocator_free(void* addr) {
  return allocator::free(addr);
}

}  // extern "C"
