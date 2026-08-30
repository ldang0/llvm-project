// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -std=gnu17 \
// RUN:   -Wno-atomic-alignment -c %s -o %t.o
// RUN: llvm-nm --undefined-only %t.o | FileCheck %s

// CHECK-DAG: U __atomic_exchange
// CHECK-DAG: U __atomic_load
// CHECK-DAG: U __atomic_store
// CHECK-NOT: __scoped

struct ThreeBytes {
  unsigned char bytes[3];
};

void scoped_fallback_load(struct ThreeBytes *ptr, struct ThreeBytes *result) {
  __scoped_atomic_load(ptr, result, __ATOMIC_ACQUIRE,
                       __MEMORY_SCOPE_DEVICE);
}

void scoped_fallback_store(struct ThreeBytes *ptr,
                           struct ThreeBytes *value) {
  __scoped_atomic_store(ptr, value, __ATOMIC_RELEASE,
                        __MEMORY_SCOPE_WRKGRP);
}

void scoped_fallback_exchange(struct ThreeBytes *ptr,
                              struct ThreeBytes *value,
                              struct ThreeBytes *result, int scope) {
  __scoped_atomic_exchange(ptr, value, result, __ATOMIC_SEQ_CST, scope);
}
