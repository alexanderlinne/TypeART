// RUN: %run %s 2>&1 | %filecheck %s
// XFAIL: *

// Currently fails because geps with an integer source type
// are not handled.

// When using multiple inheritance, the unoptimized code uses
// a cast to i8 to get the pointer for struct B and then a
// gep with index 0 to get the member k.
// The optimized code uses a gep with indices 0, 1, 0 as one
// would usually expect.
// With opaque pointes and optimization enabled, only an i8
// gep remains.

struct A {
  double i;
};

struct B {
  int* k;
};

struct S : A, B {
  int j;
};

void test_fn(S* s) {
  s->k = new int;
}

int main() {
  S s;
  test_fn(&s);
}
