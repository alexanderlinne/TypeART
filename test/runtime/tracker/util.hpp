#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <typeart/TypeART.hpp>

// Declare some of TypeART's internal APIs
extern "C" {
void typeart_tracker_alloc(const void* addr, meta_id_value alloc_id, size_t count);
void typeart_tracker_free(const void* addr);
void typeart_tracker_alloc_stack(const void* addr, meta_id_value alloc_id, size_t count);
void typeart_tracker_leave_scope(int alloca_count);
}
namespace typeart {
meta::Database& getDatabase();
}

meta::meta_id_t create_fake_double_heap_allocation() {
  double* d                = (double*)malloc(sizeof(double));
  auto pointer_info_result = typeart::PointerInfo::get(d);
  if (pointer_info_result.has_error()) {
    fprintf(stderr, "Error: could not create fake allocation!\n");
    abort();
  }
  auto pointer_info = pointer_info_result.value();
  free(d);
  return pointer_info.getAllocation().get_id();
}

meta::meta_id_t create_fake_double_stack_allocation() {
  using namespace typeart;
  double d;
  auto pointer_info_result = PointerInfo::get(&d);
  if (pointer_info_result.has_error()) {
    fprintf(stderr, "Error: could not create fake allocation!\n");
    abort();
  }
  auto pointer_info = pointer_info_result.value();
  auto instr_alloc  = meta::dyn_cast<meta::StackAllocation>(&pointer_info.getAllocation());
  assert(instr_alloc != nullptr);
  auto stack_alloc = std::make_unique<meta::StackAllocation>();
  stack_alloc->set_local_variable_raw(instr_alloc->get_local_variable_raw());
  stack_alloc->set_location_raw(instr_alloc->get_location_raw());
  auto meta     = std::make_unique<meta::Optional>();
  auto optional = meta.get();
  getDatabase().addMeta(std::move(meta));
  stack_alloc->set_count_raw(optional);
  auto result = stack_alloc.get();
  getDatabase().addMeta(std::move(stack_alloc));
  return result->get_id();
}

void check(void* addr, const char* type_name, int count, bool resolveStructs) {
  auto pointer_info_result = typeart::PointerInfo::get(addr);
  if (pointer_info_result.has_value()) {
    auto pointer_info = std::move(pointer_info_result).value();
    if (resolveStructs) {
      auto resolved_result = pointer_info.resolveStructureOrArrayType();
      while (resolved_result.has_value()) {
        pointer_info    = std::move(resolved_result).value();
        resolved_result = pointer_info.resolveStructureOrArrayType();
      }
    }

    const auto pretty_name = pointer_info.getType().get_pretty_name();
    if (pretty_name == type_name) {
      if (pointer_info.getCount() != count) {
        fprintf(stderr, "Error: Count mismatch, expected: %d, actual: %zu\n", count, pointer_info.getCount());
      } else {
        fprintf(stderr, "Ok\n");
      }
    } else {
      fprintf(stderr, "Error: Type mismatch, expected: %s, actual: %s\n", type_name, pretty_name.c_str());
    }

  } else {
    switch (pointer_info_result.error()) {
      case typeart::Status::UNKNOWN_ADDRESS:
        fprintf(stderr, "Error: Unknown address\n");
        break;
      case typeart::Status::BAD_ALIGNMENT:
        fprintf(stderr, "Error: Bad alignment\n");
        break;
      default:
        fprintf(stderr, "Error: Unexpected status: %d\n", pointer_info_result.error());
        break;
    }
  }
}

void check_struct(void* addr, const char* name, int expected_count) {
  auto pointer_info_result = typeart::PointerInfo::get(addr);
  if (pointer_info_result.has_value()) {
    const auto pointer_info = std::move(pointer_info_result).value();
    if (pointer_info.getType().is_structure_type()) {
      const auto pretty_name = pointer_info.getType().get_pretty_name();
      if (name != pretty_name) {
        fprintf(stderr, "Error: Name mismatch, expected: %s, actual: %s\n", name, pretty_name.c_str());
      } else if (expected_count != pointer_info.getCount()) {
        fprintf(stderr, "Error: Count mismatch, expected: %d, actual: %zu\n", expected_count, pointer_info.getCount());
      } else {
        fprintf(stderr, "Ok\n");
      }
    } else {
      fprintf(stderr, "Error: Not a struct\n");
    }
  } else {
    switch (pointer_info_result.error()) {
      case typeart::Status::UNKNOWN_ADDRESS:
        fprintf(stderr, "Error: Unknown address\n");
        break;
      case typeart::Status::BAD_ALIGNMENT:
        fprintf(stderr, "Error: Bad alignment\n");
        break;
      default:
        fprintf(stderr, "Error: Unexpected status: %d\n", pointer_info_result.error());
        break;
    }
  }
}

#endif