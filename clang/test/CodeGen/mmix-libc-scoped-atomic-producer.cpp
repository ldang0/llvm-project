// REQUIRES: mmix-registered-target
// RUN: %clang --target=mmix-unknown-unknown -std=c++17 -ffreestanding \
// RUN:   -fno-builtin -fno-exceptions -fno-rtti -fno-unwind-tables \
// RUN:   -fno-asynchronous-unwind-tables -fno-lax-vector-conversions \
// RUN:   -ftrivial-auto-var-init=pattern -fno-pic -fno-pie -nostdinc++ \
// RUN:   -DLIBC_FULL_BUILD -DLIBC_NAMESPACE=__llvm_libc \
// RUN:   -DLIBC_THREAD_MODE=1 -I %S/../../../libc -O1 -c %s -o %t.single.o
// RUN: llvm-nm --undefined-only %t.single.o | count 0
// RUN: %clang --target=mmix-unknown-unknown -std=c++17 -ffreestanding \
// RUN:   -fno-builtin -fno-exceptions -fno-rtti -fno-unwind-tables \
// RUN:   -fno-asynchronous-unwind-tables -fno-lax-vector-conversions \
// RUN:   -ftrivial-auto-var-init=pattern -fno-pic -fno-pie -nostdinc++ \
// RUN:   -DLIBC_FULL_BUILD -DLIBC_NAMESPACE=__llvm_libc \
// RUN:   -DLIBC_THREAD_MODE=0 -I %S/../../../libc -O1 -S -emit-llvm %s -o - \
// RUN:   | FileCheck %s --implicit-check-not='syncscope(' \
// RUN:   --implicit-check-not=__atomic_
// RUN: %clang --target=mmix-unknown-unknown -std=c++17 -ffreestanding \
// RUN:   -fno-builtin -fno-exceptions -fno-rtti -fno-unwind-tables \
// RUN:   -fno-asynchronous-unwind-tables -fno-lax-vector-conversions \
// RUN:   -ftrivial-auto-var-init=pattern -fno-pic -fno-pie -nostdinc++ \
// RUN:   -DLIBC_FULL_BUILD -DLIBC_NAMESPACE=__llvm_libc \
// RUN:   -DLIBC_THREAD_MODE=0 -I %S/../../../libc -O1 -c %s -o %t.platform.o
// RUN: llvm-nm --undefined-only %t.platform.o | count 0
// RUN: %clang --target=mmix-unknown-unknown -std=c++17 -ffreestanding \
// RUN:   -fno-builtin -fno-exceptions -fno-rtti -fno-unwind-tables \
// RUN:   -fno-asynchronous-unwind-tables -fno-lax-vector-conversions \
// RUN:   -ftrivial-auto-var-init=pattern -fno-pic -fno-pie -nostdinc++ \
// RUN:   -DLIBC_FULL_BUILD -DLIBC_NAMESPACE=__llvm_libc \
// RUN:   -DLIBC_THREAD_MODE=1 -I %S/../../../libc -O1 \
// RUN:   -c %S/../../../libc/src/stdlib/rand_util.cpp -o %t.rand-util.single.o
// RUN: %clang --target=mmix-unknown-unknown -std=c++17 -ffreestanding \
// RUN:   -fno-builtin -fno-exceptions -fno-rtti -fno-unwind-tables \
// RUN:   -fno-asynchronous-unwind-tables -fno-lax-vector-conversions \
// RUN:   -ftrivial-auto-var-init=pattern -fno-pic -fno-pie -nostdinc++ \
// RUN:   -DLIBC_FULL_BUILD -DLIBC_NAMESPACE=__llvm_libc \
// RUN:   -DLIBC_THREAD_MODE=1 -I %S/../../../libc -O1 \
// RUN:   -c %S/../../../libc/src/stdlib/rand.cpp -o %t.rand.single.o
// RUN: %clang --target=mmix-unknown-unknown -std=c++17 -ffreestanding \
// RUN:   -fno-builtin -fno-exceptions -fno-rtti -fno-unwind-tables \
// RUN:   -fno-asynchronous-unwind-tables -fno-lax-vector-conversions \
// RUN:   -ftrivial-auto-var-init=pattern -fno-pic -fno-pie -nostdinc++ \
// RUN:   -DLIBC_FULL_BUILD -DLIBC_NAMESPACE=__llvm_libc \
// RUN:   -DLIBC_THREAD_MODE=1 -I %S/../../../libc -O1 \
// RUN:   -c %S/../../../libc/src/stdlib/srand.cpp -o %t.srand.single.o
// RUN: llvm-ar rc %t.single.a %t.rand-util.single.o %t.rand.single.o \
// RUN:   %t.srand.single.o
// RUN: llvm-ar t %t.single.a | FileCheck %s --check-prefix=ARCHIVE
// RUN: llvm-nm --undefined-only %t.single.a \
// RUN:   | FileCheck %s --check-prefix=UNDEFINED \
// RUN:   --implicit-check-not=__atomic_ --implicit-check-not=__scoped
// RUN: %clang --target=mmix-unknown-unknown -std=c++17 -ffreestanding \
// RUN:   -fno-builtin -fno-exceptions -fno-rtti -fno-unwind-tables \
// RUN:   -fno-asynchronous-unwind-tables -fno-lax-vector-conversions \
// RUN:   -ftrivial-auto-var-init=pattern -fno-pic -fno-pie -nostdinc++ \
// RUN:   -DLIBC_FULL_BUILD -DLIBC_NAMESPACE=__llvm_libc \
// RUN:   -DLIBC_THREAD_MODE=0 -I %S/../../../libc -O1 \
// RUN:   -c %S/../../../libc/src/stdlib/rand_util.cpp -o %t.rand-util.platform.o
// RUN: %clang --target=mmix-unknown-unknown -std=c++17 -ffreestanding \
// RUN:   -fno-builtin -fno-exceptions -fno-rtti -fno-unwind-tables \
// RUN:   -fno-asynchronous-unwind-tables -fno-lax-vector-conversions \
// RUN:   -ftrivial-auto-var-init=pattern -fno-pic -fno-pie -nostdinc++ \
// RUN:   -DLIBC_FULL_BUILD -DLIBC_NAMESPACE=__llvm_libc \
// RUN:   -DLIBC_THREAD_MODE=0 -I %S/../../../libc -O1 \
// RUN:   -c %S/../../../libc/src/stdlib/rand.cpp -o %t.rand.platform.o
// RUN: %clang --target=mmix-unknown-unknown -std=c++17 -ffreestanding \
// RUN:   -fno-builtin -fno-exceptions -fno-rtti -fno-unwind-tables \
// RUN:   -fno-asynchronous-unwind-tables -fno-lax-vector-conversions \
// RUN:   -ftrivial-auto-var-init=pattern -fno-pic -fno-pie -nostdinc++ \
// RUN:   -DLIBC_FULL_BUILD -DLIBC_NAMESPACE=__llvm_libc \
// RUN:   -DLIBC_THREAD_MODE=0 -I %S/../../../libc -O1 \
// RUN:   -c %S/../../../libc/src/stdlib/srand.cpp -o %t.srand.platform.o
// RUN: llvm-ar rc %t.platform.a %t.rand-util.platform.o %t.rand.platform.o \
// RUN:   %t.srand.platform.o
// RUN: llvm-ar t %t.platform.a | FileCheck %s --check-prefix=ARCHIVE
// RUN: llvm-nm --undefined-only %t.platform.a \
// RUN:   | FileCheck %s --check-prefix=UNDEFINED \
// RUN:   --implicit-check-not=__atomic_ --implicit-check-not=__scoped

#include "src/__support/CPP/atomic.h"

namespace cpp = LIBC_NAMESPACE::cpp;
using Atomic = cpp::Atomic<unsigned long>;

// CHECK-LABEL: define{{.*}} i64 @producer_load(
// CHECK: load atomic i64, ptr %{{.*}} seq_cst, align 8
extern "C" unsigned long producer_load(Atomic *value) { return value->load(); }

// CHECK-LABEL: define{{.*}} void @producer_store(
// CHECK: store atomic i64 %{{.*}}, ptr %{{.*}} release, align 8
extern "C" void producer_store(Atomic *value, unsigned long desired) {
  value->store(desired, cpp::MemoryOrder::RELEASE);
}

// CHECK-LABEL: define{{.*}} i64 @producer_exchange(
// CHECK: atomicrmw xchg ptr %{{.*}}, i64 %{{.*}} acq_rel, align 8
extern "C" unsigned long producer_exchange(Atomic *value,
                                             unsigned long desired) {
  return value->exchange(desired, cpp::MemoryOrder::ACQ_REL);
}

// CHECK-LABEL: define{{.*}} i64 @producer_fetch_add(
// CHECK: atomicrmw add ptr %{{.*}}, i64 %{{.*}} monotonic, align 8
extern "C" unsigned long producer_fetch_add(Atomic *value,
                                              unsigned long operand) {
  return value->fetch_add(operand, cpp::MemoryOrder::RELAXED);
}

// CHECK-LABEL: define{{.*}} i64 @producer_fetch_sub(
// CHECK: atomicrmw sub ptr %{{.*}}, i64 %{{.*}} acquire, align 8
extern "C" unsigned long producer_fetch_sub(Atomic *value,
                                              unsigned long operand) {
  return value->fetch_sub(operand, cpp::MemoryOrder::ACQUIRE);
}

// CHECK-LABEL: define{{.*}} i64 @producer_fetch_and(
// CHECK: atomicrmw and ptr %{{.*}}, i64 %{{.*}} release, align 8
extern "C" unsigned long producer_fetch_and(Atomic *value,
                                              unsigned long operand) {
  return value->fetch_and(operand, cpp::MemoryOrder::RELEASE);
}

// CHECK-LABEL: define{{.*}} i64 @producer_fetch_or(
// CHECK: atomicrmw or ptr %{{.*}}, i64 %{{.*}} seq_cst, align 8
extern "C" unsigned long producer_fetch_or(Atomic *value,
                                             unsigned long operand) {
  return value->fetch_or(operand, cpp::MemoryOrder::SEQ_CST);
}

// CHECK-LABEL: define{{.*}} i1 @producer_compare_exchange(
// CHECK: cmpxchg ptr %{{.*}}, i64 %{{.*}} seq_cst seq_cst, align 8
extern "C" bool producer_compare_exchange(Atomic *value,
                                            unsigned long *expected,
                                            unsigned long desired) {
  return value->compare_exchange_strong(*expected, desired);
}

// CHECK-LABEL: define{{.*}} void @producer_fence(
// CHECK: fence seq_cst
extern "C" void producer_fence() {
  cpp::atomic_thread_fence(cpp::MemoryOrder::SEQ_CST);
}

// ARCHIVE: rand-util
// ARCHIVE-NEXT: rand
// ARCHIVE-NEXT: srand
// UNDEFINED: U _ZN11__llvm_libc9rand_nextE
