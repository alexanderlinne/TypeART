// clang-format off
// RUN: %c-to-llvm -fno-discard-value-names %omp_c_flags %s | %apply-typeart -typeart-stack -typeart-call-filter -S 2>&1 | %filecheck %s
// RUN: %c-to-llvm -fno-discard-value-names %omp_c_flags %s | %opt -O2 -S | %apply-typeart -typeart-stack -typeart-call-filter -S 2>&1 | %filecheck %s --check-prefix=CHECK-opt

// RUN: %c-to-llvm -fno-discard-value-names %omp_c_flags %s | %apply-typeart -typeart-stack -typeart-call-filter -S | %filecheck %s --check-prefix=check-inst
// REQUIRES: openmp
// REQUIRES: tracker
// clang-format on
extern void MPI_call(void*);

void foo() {
  // check-inst: define {{.*}} @foo
  // check-inst: %x = alloca
  // check-inst: %[[VALUE:[0-9]*]] = bitcast i32* %x to i8*
  // check-inst: call void @typeart_tracker_alloc_stack(i8* %[[VALUE]], i32 {{[0-9]*}}, i64 1)
  // check-inst-not: typeart_tracker_alloc_stack_omp
  int x;
#pragma omp parallel
  {
#pragma omp task
    { MPI_call(&x); }
  }
}

// FIXME one alloca is of the anon struct detected as OMP task struct related (need refinement of condition?)
// The Pattern: a = alloca struct; b = task_alloc; mem_cpy a to b;
// CHECK: TypeArtPass [Heap & Stack]
// CHECK-NEXT: Malloc :   0
// CHECK-NEXT: Free   :   0
// CHECK-NEXT: Alloca :   2
// CHECK-NEXT: Global :   0

// CHECK-opt: TypeArtPass [Heap & Stack]
// CHECK-opt-NEXT: Malloc :   0
// CHECK-opt-NEXT: Free   :   0
// CHECK-opt-NEXT: Alloca :   1
// CHECK-opt-NEXT: Global :   0