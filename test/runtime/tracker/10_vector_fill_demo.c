// RUN: %run %s 2>&1 | %filecheck %s
// REQUIRES: tracker

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <typeart/TypeART.h>

typedef struct vector_t {
  double* vals;
  int size;
} vector;

vector alloc_vector(int n) {
  vector v;
  v.size = n;
  v.vals = malloc(n * sizeof(double));
  return v;
}

void free_vector(vector v) {
  free(v.vals);
}

int fill_vector(void* values, int count, vector* v) {
  typeart_pointer_info pointer_info;
  typeart_status result = typeart_get_pointer_info(values, &pointer_info);
  if (result == TYPEART_OK && pointer_info.type_id == TYPEART_DOUBLE) {
    memcpy(v->vals, values, count);
    v->size = count;
    fprintf(stderr, "Success\n");
    return 1;
  }
  fprintf(stderr, "Failure\n");
  return 0;
}

int main(int argc, char** argv) {
  const int n = 3;
  // CHECK: [Trace] TypeART Runtime Trace
  // CHECK: [Trace] Alloc 0x{{.*}} int32 4 3
  int int_vals[3] = {1, 2, 3};
  // CHECK: [Trace] Alloc 0x{{.*}} double 8 3
  double d_vals[3] = {1, 2, 3};
  // CHECK: [Trace] Alloc 0x{{.*}} float 4 3
  float f_vals[3] = {1, 2, 3};
  // CHECK: [Trace] Alloc 0x{{.*}} double 8 3
  vector v = alloc_vector(n);
  // CHECK: [Trace] Alloc 0x{{.*}} double 8 3
  vector w = alloc_vector(n);
  // CHECK: Success
  fill_vector(w.vals, n, &v);
  // CHECK: Failure
  fill_vector(int_vals, n, &v);
  // CHECK: Success
  fill_vector(d_vals, n, &v);
  // CHECK: Failure
  fill_vector(f_vals, n, &v);
  // CHECK: [Trace] Free 0x{{.*}}
  free_vector(w);
  // CHECK: [Trace] Free 0x{{.*}}
  free_vector(v);
  return 0;
}
