#include "runtime/TypeResolution.hpp"

#include <optional>
#include <pthread.h>

namespace typeart::runtime::allocator {

void* malloc(alloc_id_t alloc_id, size_t count, size_t size);
bool free(void* addr);

std::optional<PointerInfo> getPointerInfo(const void* addr);

namespace stack {

void* allocate(pthread_t new_owner);
bool is_owner(pthread_t thread);
bool is_instrumented(void* addr);
void free(pthread_t current_owner);

}  // namespace stack

}  // namespace typeart::runtime::allocator
