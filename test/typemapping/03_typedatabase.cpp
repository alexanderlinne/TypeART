// clang-format off
// RUN: %apply %s --manual --object %s.o --compile_flags "-std=c++17"
// RUN: %clang-cpp %s.o -o %s.exe %types_lib -Wl,-rpath,%typeslib_path
// RUN: %s.exe | %filecheck %s

// RUN: %run %s --manual --compile_flags "-std=c++17" | %filecheck %s --check-prefix=RUNTIME-LINK

// UNSUPPORTED: sanitizer
// UNSUPPORTED: coverage

// clang-format on

#include <cstdio>
#include <typeart/typelib/TypeDatabase.hpp>

int main() {
  auto db = typeart::make_database("types-missing-file.yaml");
  printf("isValid: %i\n", db.first->isValid(typeart::type_id_t{256}));
  printf("isValid: %i\n", db.first->isValid(typeart::type_id_t{1}));
  printf("isValid: %i\n", db.first->isValid(typeart::type_id_t{258}));

  return 0;
}

// CHECK: 0
// CHECK-NEXT: 1
// CHECK-NEXT: 0

// RUNTIME-LINK: 0
// RUNTIME-LINK-NEXT: 1
// RUNTIME-LINK-NEXT: 0