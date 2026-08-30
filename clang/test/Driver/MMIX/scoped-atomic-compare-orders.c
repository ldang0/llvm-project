// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -std=gnu17 -O1 \
// RUN:   -S -emit-llvm %s -o - \
// RUN:   | FileCheck %s --implicit-check-not='syncscope('

// CHECK-LABEL: define{{.*}} i1 @relaxed_relaxed(
// CHECK: cmpxchg ptr %{{.*}} monotonic monotonic, align 8
// CHECK-LABEL: define{{.*}} i1 @consume_relaxed(
// CHECK: cmpxchg ptr %{{.*}} acquire monotonic, align 8
// CHECK-LABEL: define{{.*}} i1 @consume_consume(
// CHECK: cmpxchg ptr %{{.*}} acquire acquire, align 8
// CHECK-LABEL: define{{.*}} i1 @acquire_relaxed(
// CHECK: cmpxchg ptr %{{.*}} acquire monotonic, align 8
// CHECK-LABEL: define{{.*}} i1 @acquire_consume(
// CHECK: cmpxchg ptr %{{.*}} acquire acquire, align 8
// CHECK-LABEL: define{{.*}} i1 @acquire_acquire(
// CHECK: cmpxchg ptr %{{.*}} acquire acquire, align 8
// CHECK-LABEL: define{{.*}} i1 @release_relaxed(
// CHECK: cmpxchg ptr %{{.*}} release monotonic, align 8
// CHECK-LABEL: define{{.*}} i1 @acq_rel_relaxed(
// CHECK: cmpxchg ptr %{{.*}} acq_rel monotonic, align 8
// CHECK-LABEL: define{{.*}} i1 @acq_rel_consume(
// CHECK: cmpxchg ptr %{{.*}} acq_rel acquire, align 8
// CHECK-LABEL: define{{.*}} i1 @acq_rel_acquire(
// CHECK: cmpxchg ptr %{{.*}} acq_rel acquire, align 8
// CHECK-LABEL: define{{.*}} i1 @seq_cst_relaxed(
// CHECK: cmpxchg ptr %{{.*}} seq_cst monotonic, align 8
// CHECK-LABEL: define{{.*}} i1 @seq_cst_consume(
// CHECK: cmpxchg ptr %{{.*}} seq_cst acquire, align 8
// CHECK-LABEL: define{{.*}} i1 @seq_cst_acquire(
// CHECK: cmpxchg ptr %{{.*}} seq_cst acquire, align 8
// CHECK-LABEL: define{{.*}} i1 @seq_cst_seq_cst(
// CHECK: cmpxchg ptr %{{.*}} seq_cst seq_cst, align 8

typedef unsigned long u64;

#define COMPARE_ORDER(NAME, SUCCESS, FAILURE)                                \
  _Bool NAME(u64 *ptr, u64 *expected, u64 desired) {                         \
    return __scoped_atomic_compare_exchange_n(                               \
        ptr, expected, desired, 0, SUCCESS, FAILURE,                         \
        __MEMORY_SCOPE_DEVICE);                                              \
  }

COMPARE_ORDER(relaxed_relaxed, __ATOMIC_RELAXED, __ATOMIC_RELAXED)
COMPARE_ORDER(consume_relaxed, __ATOMIC_CONSUME, __ATOMIC_RELAXED)
COMPARE_ORDER(consume_consume, __ATOMIC_CONSUME, __ATOMIC_CONSUME)
COMPARE_ORDER(acquire_relaxed, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)
COMPARE_ORDER(acquire_consume, __ATOMIC_ACQUIRE, __ATOMIC_CONSUME)
COMPARE_ORDER(acquire_acquire, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE)
COMPARE_ORDER(release_relaxed, __ATOMIC_RELEASE, __ATOMIC_RELAXED)
COMPARE_ORDER(acq_rel_relaxed, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED)
COMPARE_ORDER(acq_rel_consume, __ATOMIC_ACQ_REL, __ATOMIC_CONSUME)
COMPARE_ORDER(acq_rel_acquire, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)
COMPARE_ORDER(seq_cst_relaxed, __ATOMIC_SEQ_CST, __ATOMIC_RELAXED)
COMPARE_ORDER(seq_cst_consume, __ATOMIC_SEQ_CST, __ATOMIC_CONSUME)
COMPARE_ORDER(seq_cst_acquire, __ATOMIC_SEQ_CST, __ATOMIC_ACQUIRE)
COMPARE_ORDER(seq_cst_seq_cst, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)
