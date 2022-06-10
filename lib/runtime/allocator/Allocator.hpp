#include "runtime/Runtime.hpp"

#include <optional>
#include <pthread.h>

namespace typeart::allocator {

void* malloc(meta_id_t meta_id, size_t count, size_t size);
void* realloc(meta_id_t meta_id, size_t count, void* ptr, size_t new_size);
bool free(void* addr);

std::optional<PointerInfo> getPointerInfo(const void* addr);

namespace stack {

void* allocate(pthread_t new_owner);
bool is_owner(pthread_t thread);
bool is_instrumented(void* addr);
void free(pthread_t current_owner);

}  // namespace stack

}  // namespace typeart::allocator
