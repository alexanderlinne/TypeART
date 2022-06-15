// clang-format off
// RUN: %apply %s --manual --object %s.o --compile_flags "-std=c++17"
// RUN: %clang-cpp %s.o -o %s.exe %database_lib -Wl,-rpath,%databaselib_path
// RUN: %s.exe | %filecheck %s

// RUN: %run %s --manual --compile_flags "-std=c++17" | %filecheck %s --check-prefix=RUNTIME-LINK

// UNSUPPORTED: sanitizer
// UNSUPPORTED: coverage

// clang-format on

#include <cstdio>
#include <typeart/meta/Database.hpp>

int main() {
  auto database = typeart::Meta::load("types-missing-file.yaml");
  if (database) {
    printf("hasValue: 1\n");
  } else {
    printf("hasValue: 0\n");
  }
  typeart::Meta db;
  printf("isValid: %i\n", db.isValid(typeart::type_id_t{256}));
  printf("isValid: %i\n", db.isValid(typeart::type_id_t{1}));
  printf("isValid: %i\n", db.isValid(typeart::type_id_t{258}));

  return 0;
}

// CHECK: hasValue: 0
// CHECK: isValid: 0
// CHECK-NEXT: isValid: 1
// CHECK-NEXT: isValid: 0

// RUNTIME-LINK: hasValue: 0
// RUNTIME-LINK: isValid: 0
// RUNTIME-LINK-NEXT: isValid: 1
// RUNTIME-LINK-NEXT: isValid: 0