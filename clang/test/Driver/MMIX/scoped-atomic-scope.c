// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -std=gnu17 -O1 \
// RUN:   -S -emit-llvm %s -o - \
// RUN:   | FileCheck %s --implicit-check-not='syncscope('

// CHECK-LABEL: define{{.*}} void @system_scope(
// CHECK: fence seq_cst
// CHECK-LABEL: define{{.*}} void @device_scope(
// CHECK: fence seq_cst
// CHECK-LABEL: define{{.*}} void @workgroup_scope(
// CHECK: fence seq_cst
// CHECK-LABEL: define{{.*}} void @wavefront_scope(
// CHECK: fence seq_cst
// CHECK-LABEL: define{{.*}} void @single_scope(
// CHECK: fence seq_cst
// CHECK-LABEL: define{{.*}} void @cluster_scope(
// CHECK: fence seq_cst
// CHECK-LABEL: define{{.*}} void @runtime_scope(
// CHECK: fence seq_cst
// CHECK-LABEL: define{{.*}} void @invalid_scope(
// CHECK: fence seq_cst

#define DEFINE_FENCE(NAME, SCOPE)                                             \
  void NAME(void) {                                                          \
    __scoped_atomic_thread_fence(__ATOMIC_SEQ_CST, SCOPE);                    \
  }

DEFINE_FENCE(system_scope, __MEMORY_SCOPE_SYSTEM)
DEFINE_FENCE(device_scope, __MEMORY_SCOPE_DEVICE)
DEFINE_FENCE(workgroup_scope, __MEMORY_SCOPE_WRKGRP)
DEFINE_FENCE(wavefront_scope, __MEMORY_SCOPE_WVFRNT)
DEFINE_FENCE(single_scope, __MEMORY_SCOPE_SINGLE)
DEFINE_FENCE(cluster_scope, __MEMORY_SCOPE_CLUSTR)

void runtime_scope(int scope) {
  __scoped_atomic_thread_fence(__ATOMIC_SEQ_CST, scope);
}

void invalid_scope(void) {
  __scoped_atomic_thread_fence(__ATOMIC_SEQ_CST, -1);
}
