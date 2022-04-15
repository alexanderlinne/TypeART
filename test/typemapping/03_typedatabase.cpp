// clang-format off
// RUN: %apply %s --manual --object %s.o --compile_flags "-std=c++17"
// RUN: %clang-cpp %s.o -o %s.exe %database_lib -Wl,-rpath,%databaselib_path
// RUN: %s.exe | %filecheck %s

// RUN: %run %s --manual --compile_flags "-std=c++17" | %filecheck %s --check-prefix=RUNTIME-LINK

// UNSUPPORTED: sanitizer
// UNSUPPORTED: coverage

// clang-format on

#include <cstdio>
#include <typeart/db/Database.hpp>

int main() {
  auto database = typeart::Database::load("types-missing-file.yaml");
  if (database) {
    printf("hasValue: 0\n");
  } else {
    printf("hasValue: 1\n");
  }
  typeart::Database db;
  printf("isValid: %i\n", db.isValid(typeart::type_id_t{256}));
  printf("isValid: %i\n", db.isValid(typeart::type_id_t{1}));
  printf("isValid: %i\n", db.isValid(typeart::type_id_t{258}));

  return 0;
}

// CHECK: 0
// CHECK-NEXT: 1
// CHECK-NEXT: 0

// RUNTIME-LINK: 0
// RUNTIME-LINK-NEXT: 1
// RUNTIME-LINK-NEXT: 0