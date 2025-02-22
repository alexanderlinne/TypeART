// clang-format off
// RUN: %run %s 2>&1 | %filecheck %s
// REQUIRES: tracker
// clang-format on

#include <new>

struct S1 {
  int x;
  virtual ~S1() = default;
};

void foo() {
  S1* b{nullptr};
  try {
    b = new S1[6];  // FIXME this works because size computation rounds down
  } catch (...) {
  }
  if (b != nullptr) {
    delete[] b;
  }
}

int main() {
  try {
    S1* ss = new S1[3];  // FIXME this works because size computation rounds down
    delete[] ss;         // LLVM does not call _ZdaPv here, but in destructor @_ZN2S1D0Ev
  } catch (...) {
  }

  foo();

  return 0;
}
// main()
// CHECK: [Trace] Alloc heap [[POINTER:0x[0-9a-f]+]] of type [3 x S1]
// CHECK: [Trace] Free heap [[POINTER]] of type [3 x S1]
// foo()
// CHECK: [Trace] Alloc heap [[POINTER:0x[0-9a-f]+]] of type [6 x S1]
// CHECK: [Trace] Free heap [[POINTER]] of type [6 x S1]
