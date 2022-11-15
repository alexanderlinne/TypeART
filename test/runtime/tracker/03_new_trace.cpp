// RUN: %run %s 2>&1 | %filecheck %s
// REQUIRES: tracker

template <typename T>
void new_delete() {
  T* t = new T;
  delete t;
}

template <typename T>
void new_delete(int n) {
  T* t = new T[n];
  delete[] t;
}

int main(int argc, char** argv) {
  const int n = 42;

  // CHECK: [Trace] TypeART Runtime Trace

  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [1 x char]
  // CHECK: [Trace] Free heap 0x{{.*}} of type [1 x char]
  new_delete<char>();

  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [1 x short]
  // CHECK: [Trace] Free heap 0x{{.*}} of type [1 x short]
  new_delete<short>();

  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [1 x int]
  // CHECK: [Trace] Free heap 0x{{.*}} of type [1 x int]
  new_delete<int>();

  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [1 x long int]
  // CHECK: [Trace] Free heap 0x{{.*}} of type [1 x long int]
  new_delete<long>();

  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [1 x float]
  // CHECK: [Trace] Free heap 0x{{.*}} of type [1 x float]
  new_delete<float>();

  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [1 x double]
  // CHECK: [Trace] Free heap 0x{{.*}} of type [1 x double]
  new_delete<double>();

  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [1 x int*]
  // CHECK: [Trace] Free heap 0x{{.*}} of type [1 x int*]
  new_delete<int*>();

  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [42 x char]
  // CHECK: [Trace] Free heap 0x{{.*}} of type [42 x char]
  new_delete<char>(n);

  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [42 x short]
  // CHECK: [Trace] Free heap 0x{{.*}} of type [42 x short]
  new_delete<short>(n);

  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [42 x int]
  // CHECK: [Trace] Free heap 0x{{.*}} of type [42 x int]
  new_delete<int>(n);

  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [42 x long int]
  // CHECK: [Trace] Free heap 0x{{.*}} of type [42 x long int]
  new_delete<long>(n);

  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [42 x float]
  // CHECK: [Trace] Free heap 0x{{.*}} of type [42 x float]
  new_delete<float>(n);

  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [42 x double]
  // CHECK: [Trace] Free heap 0x{{.*}} of type [42 x double]
  new_delete<double>(n);

  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [42 x int*]
  // CHECK: [Trace] Free heap 0x{{.*}} of type [42 x int*]
  new_delete<int*>(n);

  return 0;
}
