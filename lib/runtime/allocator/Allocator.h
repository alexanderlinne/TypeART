#include "RuntimeData.h"
#include "RuntimeInterface.h"

#include <optional>
#include <pthread.h>

extern "C" {

void typeart_setup_main_stack(int argc, char** argv, char** envp);
}

namespace typeart::allocator {

void* malloc(int allocation_id, size_t count, size_t size);
bool free(void* addr);

struct AllocationInfo {
  PointerInfo pointer_info;
  const void* base_addr;
};

std::optional<AllocationInfo> getAllocationInfo(const void* addr);

namespace stack {

void* allocate(pthread_t new_owner);
void set_owner(const void* owned_ptr, pthread_t new_owner);
void free(pthread_t current_owner);

}  // namespace stack

}  // namespace typeart::allocator
