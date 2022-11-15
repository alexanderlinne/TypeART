// RUN: %run %s 2>&1 | %filecheck %s
// REQUIRES: tracker

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <typeart/TypeART.hpp>

using namespace typeart;

typedef struct vector_t {
  double* vals;
  int size;
} vector;

vector alloc_vector(int n) {
  vector v;
  v.size = n;
  v.vals = (double*)malloc(n * sizeof(double));
  return v;
}

void free_vector(vector v) {
  free(v.vals);
}

int fill_vector(void* values, int count, vector* v) {
  auto pointer_info_result = PointerInfo::get(values);
  if (pointer_info_result.has_error()) {
    fprintf(stderr, "Failure\n");
    return 0;
  }
  auto pointer_info = pointer_info_result.value().resolveAllArrayTypes();
  if (auto basic_type = meta::dyn_cast<meta::di::BasicType>(&pointer_info.getType());
      basic_type != nullptr && basic_type->get_encoding() == meta::di::Encoding::Float &&
      basic_type->get_size_in_bits() == 64) {
    memcpy(v->vals, values, count);
    v->size = count;
    fprintf(stderr, "Success\n");
    return 1;
  }
  fprintf(stderr, "Failure\n");
  return 0;
}

int main(int argc, char** argv) {
  const int n      = 3;
  // CHECK: [Trace] TypeART Runtime Trace
  // CHECK: [Trace] Alloc stack 0x{{.*}} of type [1 x int[3]]
  int int_vals[3]  = {1, 2, 3};
  // CHECK: [Trace] Alloc stack 0x{{.*}} of type [1 x double[3]]
  double d_vals[3] = {1, 2, 3};
  // CHECK: [Trace] Alloc stack 0x{{.*}} of type [1 x float[3]]
  float f_vals[3]  = {1, 2, 3};
  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [3 x double]
  vector v         = alloc_vector(n);
  // CHECK: [Trace] Alloc heap 0x{{.*}} of type [3 x double]
  vector w         = alloc_vector(n);
  // CHECK: Success
  fill_vector(w.vals, n, &v);
  // CHECK: Failure
  fill_vector(int_vals, n, &v);
  // CHECK: Success
  fill_vector(d_vals, n, &v);
  // CHECK: Failure
  fill_vector(f_vals, n, &v);
  // CHECK: [Trace] Free heap 0x{{.*}} of type [3 x double]
  free_vector(w);
  // CHECK: [Trace] Free heap 0x{{.*}} of type [3 x double]
  free_vector(v);
  return 0;
}
