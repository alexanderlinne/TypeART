#include "RuntimeData.h"
#include "RuntimeInterface.h"

extern "C" {

void typeart_setup_main_stack(int argc, char** argv, char** envp);
}

namespace typeart::allocator {

void* malloc(int type_id, size_t count, size_t size);
bool free(void* addr);

struct AllocationInfo {
  PointerInfo pointer_info;
  void* base_addr;
};

std::optional<AllocationInfo> getAllocationInfo(const void* addr);

}  // namespace typeart::allocator
