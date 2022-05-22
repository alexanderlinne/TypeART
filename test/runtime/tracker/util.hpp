#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <typeart/TypeART.hpp>

void check(void* addr, const char* type_name, int count, bool resolveStructs) {
  auto pointer_info_result = typeart::runtime::PointerInfo::get(addr);
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
  auto pointer_info_result = typeart::runtime::PointerInfo::get(addr);
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