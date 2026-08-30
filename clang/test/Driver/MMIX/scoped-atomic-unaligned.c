// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -std=gnu17 \
// RUN:   -Wno-address-of-packed-member -c %s -o %t.o
// RUN: llvm-nm --undefined-only %t.o | FileCheck %s

// CHECK-DAG: U __atomic_compare_exchange
// CHECK-DAG: U __atomic_exchange
// CHECK-DAG: U __atomic_load
// CHECK-NOT: __scoped

struct __attribute__((packed)) Packed {
  unsigned char padding;
  unsigned value;
};

unsigned scoped_unaligned_load(struct Packed *ptr) {
  return __scoped_atomic_load_n(&ptr->value, __ATOMIC_ACQUIRE,
                                __MEMORY_SCOPE_DEVICE);
}

unsigned scoped_unaligned_exchange(struct Packed *ptr, unsigned value) {
  return __scoped_atomic_exchange_n(&ptr->value, value, __ATOMIC_ACQ_REL,
                                    __MEMORY_SCOPE_WRKGRP);
}

unsigned scoped_unaligned_fetch_add(struct Packed *ptr, unsigned value) {
  return __scoped_atomic_fetch_add(&ptr->value, value, __ATOMIC_RELAXED,
                                   __MEMORY_SCOPE_WVFRNT);
}

_Bool scoped_unaligned_compare(struct Packed *ptr, unsigned *expected,
                               unsigned desired) {
  return __scoped_atomic_compare_exchange_n(
      &ptr->value, expected, desired, 0, __ATOMIC_SEQ_CST, __ATOMIC_ACQUIRE,
      __MEMORY_SCOPE_CLUSTR);
}
