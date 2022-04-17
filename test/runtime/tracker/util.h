#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <typeart/TypeART.h>

int get_struct_id(int index) {
  return TYPEART_NUM_RESERVED_IDS + index;
}

void check(void* addr, int id, int expected_count, int resolveStructs) {
  typeart_pointer_info pointer_info;
  typeart_status status = typeart_get_pointer_info(addr, &pointer_info);
  if (status == TYPEART_OK) {
    if (resolveStructs) {
      // If the address corresponds to a struct, fetch the type of the first member
      while (pointer_info.type_id >= TYPEART_NUM_RESERVED_IDS) {
        typeart_struct_layout struct_layout;
        typeart_resolve_type_id(pointer_info.type_id, &struct_layout);
        pointer_info.type_id = struct_layout.member_types[0];
        pointer_info.count   = struct_layout.count[0];
      }
    }

    if (pointer_info.type_id == id) {
      if (pointer_info.count != expected_count) {
        fprintf(stderr, "Error: Count mismatch (%zu)\n", pointer_info.count);
      } else {
        fprintf(stderr, "Ok\n");
      }
    } else {
      fprintf(stderr, "Error: Type mismatch\n");
    }

  } else {
    switch (status) {
      case TYPEART_UNKNOWN_ADDRESS:
        fprintf(stderr, "Error: Unknown address\n");
        break;
      case TYPEART_BAD_ALIGNMENT:
        fprintf(stderr, "Error: Bad alignment\n");
        break;
      default:
        fprintf(stderr, "Error: Unexpected status: %d\n", status);
        break;
    }
  }
}

void check_struct(void* addr, const char* name, int expected_count) {
  typeart_pointer_info pointer_info;
  typeart_status status = typeart_get_pointer_info(addr, &pointer_info);
  if (status == TYPEART_OK) {
    if (pointer_info.type_id >= TYPEART_NUM_RESERVED_IDS) {
      typeart_struct_layout struct_layout;
      typeart_resolve_type_id(pointer_info.type_id, &struct_layout);
      if (strcmp(typeart_get_type_name(pointer_info.type_id), struct_layout.name) != 0) {
        fprintf(stderr, "Error: Name mismatch\n");
      } else if (expected_count != pointer_info.count) {
        fprintf(stderr, "Error: Count mismatch (%zu)\n", pointer_info.count);
      } else {
        fprintf(stderr, "Ok\n");
      }
    } else {
      fprintf(stderr, "Error: Not a struct\n");
    }
  } else {
    switch (status) {
      case TYPEART_UNKNOWN_ADDRESS:
        fprintf(stderr, "Error: Unknown address\n");
        break;
      case TYPEART_BAD_ALIGNMENT:
        fprintf(stderr, "Error: Bad alignment\n");
        break;
      default:
        fprintf(stderr, "Error: Unexpected status: %d\n", status);
        break;
    }
  }
}

#endif