// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -std=c17 -O1 \
// RUN:   -S -emit-llvm %s -o - | FileCheck %s --check-prefix=IR
// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -std=c17 -O1 \
// RUN:   -S %s -o - | FileCheck %s --check-prefix=ASM \
// RUN:   --implicit-check-not=__atomic_ --implicit-check-not=__sync_
// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -std=c17 \
// RUN:   -DINVALID_ORDERS -fsyntax-only -Xclang -verify %s

// IR-LABEL: define {{.*}} @c11_exchange_relaxed_i8(
// IR: atomicrmw xchg {{.*}} i8 {{.*}} monotonic, align 1
// ASM-LABEL: c11_exchange_relaxed_i8:
// ASM: NXOR
// ASM: AND
// ASM: OR
// ASM: CSWAP

// IR-LABEL: define {{.*}} @c11_exchange_acquire_i16(
// IR: atomicrmw xchg {{.*}} i16 {{.*}} acquire, align 2
// ASM-LABEL: c11_exchange_acquire_i16:
// ASM: CSWAP
// ASM: SYNC 3

// IR-LABEL: define {{.*}} @gnu_exchange_release_i32(
// IR: atomicrmw xchg {{.*}} i32 {{.*}} release, align 4
// ASM-LABEL: gnu_exchange_release_i32:
// ASM: SYNC 3
// ASM: CSWAP

// IR-LABEL: define {{.*}} @gnu_exchange_seq_cst_i64(
// IR: atomicrmw xchg {{.*}} i64 {{.*}} seq_cst, align 8
// ASM-LABEL: gnu_exchange_seq_cst_i64:
// ASM: SYNC 3
// ASM: CSWAP
// ASM: SYNC 3

// IR-LABEL: define {{.*}} @gnu_generic_exchange(
// IR: atomicrmw xchg {{.*}} i64 {{.*}} acq_rel, align 8
// ASM-LABEL: gnu_generic_exchange:
// ASM: SYNC 3
// ASM: CSWAP
// ASM: SYNC 3

// IR-LABEL: define {{.*}} @c11_compare_strong_i8(
// IR: [[STRONG:%.*]] = cmpxchg {{.*}} i8 {{.*}} acq_rel acquire, align 1
// IR: [[STRONG_OK:%.*]] = extractvalue { i8, i1 } [[STRONG]], 1
// IR: br i1 [[STRONG_OK]],
// IR: [[STRONG_OLD:%.*]] = extractvalue { i8, i1 } [[STRONG]], 0
// IR: store i8 [[STRONG_OLD]], ptr {{.*}}, align 1
// IR: ret i1 [[STRONG_OK]]
// ASM-LABEL: c11_compare_strong_i8:
// ASM: ANDN
// ASM: NXOR
// ASM: OR
// ASM: CSWAP

// IR-LABEL: define {{.*}} @c11_compare_weak_i16(
// IR: [[WEAK:%.*]] = cmpxchg weak {{.*}} i16 {{.*}} acquire monotonic, align 2
// IR: [[WEAK_OK:%.*]] = extractvalue { i16, i1 } [[WEAK]], 1
// IR: br i1 [[WEAK_OK]],
// IR: [[WEAK_OLD:%.*]] = extractvalue { i16, i1 } [[WEAK]], 0
// IR: store i16 [[WEAK_OLD]], ptr {{.*}}, align 2
// IR: ret i1 [[WEAK_OK]]
// ASM-LABEL: c11_compare_weak_i16:
// ASM: CSWAP

// IR-LABEL: define {{.*}} @gnu_compare_weak_i32(
// IR: cmpxchg weak {{.*}} i32 {{.*}} release monotonic, align 4
// ASM-LABEL: gnu_compare_weak_i32:
// ASM: CSWAP

// IR-LABEL: define {{.*}} @gnu_generic_compare_i64(
// IR: cmpxchg {{.*}} i64 {{.*}} seq_cst seq_cst, align 8
// ASM-LABEL: gnu_generic_compare_i64:
// ASM: CSWAP

// The explicit checks below cover every success/failure pair accepted by
// Clang. C consume is represented conservatively as LLVM acquire.
// IR-LABEL: define {{.*}} @order_relaxed_relaxed(
// IR: cmpxchg {{.*}} i64 {{.*}} monotonic monotonic, align 8
// ASM-LABEL: order_relaxed_relaxed:
// ASM: CSWAP
// IR-LABEL: define {{.*}} @order_relaxed_consume(
// IR: cmpxchg {{.*}} i64 {{.*}} monotonic acquire, align 8
// ASM-LABEL: order_relaxed_consume:
// ASM: CSWAP
// IR-LABEL: define {{.*}} @order_relaxed_acquire(
// IR: cmpxchg {{.*}} i64 {{.*}} monotonic acquire, align 8
// ASM-LABEL: order_relaxed_acquire:
// ASM: CSWAP
// IR-LABEL: define {{.*}} @order_relaxed_seq_cst(
// IR: cmpxchg {{.*}} i64 {{.*}} monotonic seq_cst, align 8
// ASM-LABEL: order_relaxed_seq_cst:
// ASM: CSWAP
// IR-LABEL: define {{.*}} @order_consume_relaxed(
// IR: cmpxchg {{.*}} i64 {{.*}} acquire monotonic, align 8
// ASM-LABEL: order_consume_relaxed:
// ASM: CSWAP
// IR-LABEL: define {{.*}} @order_consume_consume(
// IR: cmpxchg {{.*}} i64 {{.*}} acquire acquire, align 8
// ASM-LABEL: order_consume_consume:
// ASM: CSWAP
// IR-LABEL: define {{.*}} @order_consume_acquire(
// IR: cmpxchg {{.*}} i64 {{.*}} acquire acquire, align 8
// ASM-LABEL: order_consume_acquire:
// ASM: CSWAP
// IR-LABEL: define {{.*}} @order_consume_seq_cst(
// IR: cmpxchg {{.*}} i64 {{.*}} acquire seq_cst, align 8
// ASM-LABEL: order_consume_seq_cst:
// ASM: CSWAP
// IR-LABEL: define {{.*}} @order_acquire_relaxed(
// IR: cmpxchg {{.*}} i64 {{.*}} acquire monotonic, align 8
// ASM-LABEL: order_acquire_relaxed:
// ASM: CSWAP
// IR-LABEL: define {{.*}} @order_acquire_consume(
// IR: cmpxchg {{.*}} i64 {{.*}} acquire acquire, align 8
// ASM-LABEL: order_acquire_consume:
// ASM: CSWAP
// IR-LABEL: define {{.*}} @order_acquire_acquire(
// IR: cmpxchg {{.*}} i64 {{.*}} acquire acquire, align 8
// ASM-LABEL: order_acquire_acquire:
// ASM: CSWAP
// IR-LABEL: define {{.*}} @order_acquire_seq_cst(
// IR: cmpxchg {{.*}} i64 {{.*}} acquire seq_cst, align 8
// ASM-LABEL: order_acquire_seq_cst:
// ASM: CSWAP
// IR-LABEL: define {{.*}} @order_release_relaxed(
// IR: cmpxchg {{.*}} i64 {{.*}} release monotonic, align 8
// ASM-LABEL: order_release_relaxed:
// ASM: CSWAP
// IR-LABEL: define {{.*}} @order_release_consume(
// IR: cmpxchg {{.*}} i64 {{.*}} release acquire, align 8
// ASM-LABEL: order_release_consume:
// ASM: CSWAP
// IR-LABEL: define {{.*}} @order_release_acquire(
// IR: cmpxchg {{.*}} i64 {{.*}} release acquire, align 8
// ASM-LABEL: order_release_acquire:
// ASM: CSWAP
// IR-LABEL: define {{.*}} @order_release_seq_cst(
// IR: cmpxchg {{.*}} i64 {{.*}} release seq_cst, align 8
// ASM-LABEL: order_release_seq_cst:
// ASM: CSWAP
// IR-LABEL: define {{.*}} @order_acq_rel_relaxed(
// IR: cmpxchg {{.*}} i64 {{.*}} acq_rel monotonic, align 8
// ASM-LABEL: order_acq_rel_relaxed:
// ASM: CSWAP
// IR-LABEL: define {{.*}} @order_acq_rel_consume(
// IR: cmpxchg {{.*}} i64 {{.*}} acq_rel acquire, align 8
// ASM-LABEL: order_acq_rel_consume:
// ASM: CSWAP
// IR-LABEL: define {{.*}} @order_acq_rel_acquire(
// IR: cmpxchg {{.*}} i64 {{.*}} acq_rel acquire, align 8
// ASM-LABEL: order_acq_rel_acquire:
// ASM: CSWAP
// IR-LABEL: define {{.*}} @order_acq_rel_seq_cst(
// IR: cmpxchg {{.*}} i64 {{.*}} acq_rel seq_cst, align 8
// ASM-LABEL: order_acq_rel_seq_cst:
// ASM: CSWAP
// IR-LABEL: define {{.*}} @order_seq_cst_relaxed(
// IR: cmpxchg {{.*}} i64 {{.*}} seq_cst monotonic, align 8
// ASM-LABEL: order_seq_cst_relaxed:
// ASM: CSWAP
// IR-LABEL: define {{.*}} @order_seq_cst_consume(
// IR: cmpxchg {{.*}} i64 {{.*}} seq_cst acquire, align 8
// ASM-LABEL: order_seq_cst_consume:
// ASM: CSWAP
// IR-LABEL: define {{.*}} @order_seq_cst_acquire(
// IR: cmpxchg {{.*}} i64 {{.*}} seq_cst acquire, align 8
// ASM-LABEL: order_seq_cst_acquire:
// ASM: CSWAP
// IR-LABEL: define {{.*}} @order_seq_cst_seq_cst(
// IR: cmpxchg {{.*}} i64 {{.*}} seq_cst seq_cst, align 8
// ASM-LABEL: order_seq_cst_seq_cst:
// ASM: CSWAP

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long u64;

u8 c11_exchange_relaxed_i8(_Atomic(u8) *ptr, u8 value) {
  return __c11_atomic_exchange(ptr, value, __ATOMIC_RELAXED);
}

u16 c11_exchange_acquire_i16(_Atomic(u16) *ptr, u16 value) {
  return __c11_atomic_exchange(ptr, value, __ATOMIC_ACQUIRE);
}

u32 gnu_exchange_release_i32(u32 *ptr, u32 value) {
  return __atomic_exchange_n(ptr, value, __ATOMIC_RELEASE);
}

u64 gnu_exchange_seq_cst_i64(u64 *ptr, u64 value) {
  return __atomic_exchange_n(ptr, value, __ATOMIC_SEQ_CST);
}

void gnu_generic_exchange(u64 *ptr, u64 *value, u64 *old) {
  __atomic_exchange(ptr, value, old, __ATOMIC_ACQ_REL);
}

_Bool c11_compare_strong_i8(_Atomic(u8) *ptr, u8 *expected, u8 desired) {
  return __c11_atomic_compare_exchange_strong(
      ptr, expected, desired, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
}

_Bool c11_compare_weak_i16(_Atomic(u16) *ptr, u16 *expected, u16 desired) {
  return __c11_atomic_compare_exchange_weak(
      ptr, expected, desired, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED);
}

_Bool gnu_compare_weak_i32(u32 *ptr, u32 *expected, u32 desired) {
  return __atomic_compare_exchange_n(ptr, expected, desired, 1,
                                     __ATOMIC_RELEASE, __ATOMIC_RELAXED);
}

_Bool gnu_generic_compare_i64(u64 *ptr, u64 *expected, u64 *desired) {
  return __atomic_compare_exchange(ptr, expected, desired, 0,
                                   __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

#define ORDER_PAIR(NAME, SUCCESS, FAILURE)                                 \
  _Bool order_##NAME(_Atomic(u64) *ptr, u64 *expected, u64 desired) {      \
    return __c11_atomic_compare_exchange_strong(ptr, expected, desired,    \
                                                 SUCCESS, FAILURE);         \
  }

ORDER_PAIR(relaxed_relaxed, __ATOMIC_RELAXED, __ATOMIC_RELAXED)
ORDER_PAIR(relaxed_consume, __ATOMIC_RELAXED, __ATOMIC_CONSUME)
ORDER_PAIR(relaxed_acquire, __ATOMIC_RELAXED, __ATOMIC_ACQUIRE)
ORDER_PAIR(relaxed_seq_cst, __ATOMIC_RELAXED, __ATOMIC_SEQ_CST)
ORDER_PAIR(consume_relaxed, __ATOMIC_CONSUME, __ATOMIC_RELAXED)
ORDER_PAIR(consume_consume, __ATOMIC_CONSUME, __ATOMIC_CONSUME)
ORDER_PAIR(consume_acquire, __ATOMIC_CONSUME, __ATOMIC_ACQUIRE)
ORDER_PAIR(consume_seq_cst, __ATOMIC_CONSUME, __ATOMIC_SEQ_CST)
ORDER_PAIR(acquire_relaxed, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)
ORDER_PAIR(acquire_consume, __ATOMIC_ACQUIRE, __ATOMIC_CONSUME)
ORDER_PAIR(acquire_acquire, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE)
ORDER_PAIR(acquire_seq_cst, __ATOMIC_ACQUIRE, __ATOMIC_SEQ_CST)
ORDER_PAIR(release_relaxed, __ATOMIC_RELEASE, __ATOMIC_RELAXED)
ORDER_PAIR(release_consume, __ATOMIC_RELEASE, __ATOMIC_CONSUME)
ORDER_PAIR(release_acquire, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE)
ORDER_PAIR(release_seq_cst, __ATOMIC_RELEASE, __ATOMIC_SEQ_CST)
ORDER_PAIR(acq_rel_relaxed, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED)
ORDER_PAIR(acq_rel_consume, __ATOMIC_ACQ_REL, __ATOMIC_CONSUME)
ORDER_PAIR(acq_rel_acquire, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)
ORDER_PAIR(acq_rel_seq_cst, __ATOMIC_ACQ_REL, __ATOMIC_SEQ_CST)
ORDER_PAIR(seq_cst_relaxed, __ATOMIC_SEQ_CST, __ATOMIC_RELAXED)
ORDER_PAIR(seq_cst_consume, __ATOMIC_SEQ_CST, __ATOMIC_CONSUME)
ORDER_PAIR(seq_cst_acquire, __ATOMIC_SEQ_CST, __ATOMIC_ACQUIRE)
ORDER_PAIR(seq_cst_seq_cst, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)

#ifdef INVALID_ORDERS
void invalid_failure_orders(_Atomic(u64) *ptr, u64 *expected, u64 desired) {
  __c11_atomic_compare_exchange_strong(
      ptr, expected, desired, __ATOMIC_SEQ_CST,
      __ATOMIC_RELEASE); // expected-warning {{failure memory order argument to atomic operation is invalid}}
  __c11_atomic_compare_exchange_strong(
      ptr, expected, desired, __ATOMIC_SEQ_CST,
      __ATOMIC_ACQ_REL); // expected-warning {{failure memory order argument to atomic operation is invalid}}
}
#endif
