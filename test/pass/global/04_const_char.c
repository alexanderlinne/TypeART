// RUN: %c-to-llvm %s | %apply-typeart -typeart-global -S 2>&1 | %filecheck %s

void bar(const void*);

void foo() {
  bar((const void*)"Hello world");
}

// CHECK: void @typeart_init_module_
// CHECK-NEXT: entry:
// CHECK-NEXT: call void @typeart_tracker_alloc_global(i8* getelementptr inbounds ([12 x i8], [12 x i8]* @.str
// CHECK-NEXT: ret void
