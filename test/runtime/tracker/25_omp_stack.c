// clang-format off
// RUN: %run %s --omp 2>&1 | %filecheck %s --check-prefix=CHECK-TSAN
// RUN: %run %s --omp 2>&1 | %filecheck %s
// REQUIRES: openmp
// REQUIRES: tracker
// clang-format on

void f() {
  char c[4];
  double d = 5;
}

int main(int argc, char** argv) {
  // CHECK: [Trace] TypeART Runtime Trace
#pragma omp parallel sections
  {
#pragma omp section
    f();
#pragma omp section
    f();
  }

  // CHECK-TSAN-NOT: ThreadSanitizer

  // CHECK-NOT: Error

  // CHECK: [Trace] Free stack 0x{{.*}} of type [1 x char[4]]
  // CHECK-DAG: [Trace] Free stack 0x{{.*}} of type [1 x double]

  // CHECK-DAG: [Trace] Free stack 0x{{.*}} of type [1 x char[4]]
  // CHECK-DAG: [Trace] Free stack 0x{{.*}} of type [1 x double]

  return 0;
}
