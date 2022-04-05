#include "RuntimeData.h"
#include "RuntimeInterface.h"

#include <optional>

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

}  // namespace typeart::allocator
