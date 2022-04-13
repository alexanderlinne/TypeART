#include "RuntimeData.h"
#include "RuntimeInterface.h"

#include <optional>
#include <pthread.h>

namespace typeart::runtime::allocator {

void* malloc(int allocation_id, size_t count, size_t size);
bool free(void* addr);

struct AllocationInfo {
  PointerInfo pointer_info;
  const void* base_addr;
};

std::optional<AllocationInfo> getAllocationInfo(const void* addr);

namespace stack {

void* allocate(pthread_t new_owner);
bool is_owner(pthread_t thread);
bool is_instrumented(void* addr);
void free(pthread_t current_owner);

}  // namespace stack

}  // namespace typeart::runtime::allocator
