// REQUIRES: mmix-registered-target
// RUN: %clang --target=mmix-unknown-unknown -std=c++17 -ffreestanding \
// RUN:   -fno-builtin -fno-exceptions -fno-rtti -fno-unwind-tables \
// RUN:   -fno-asynchronous-unwind-tables -fno-lax-vector-conversions \
// RUN:   -ftrivial-auto-var-init=pattern -DLIBC_FULL_BUILD \
// RUN:   -DLIBC_NAMESPACE=__llvm_libc -I %S/../../../libc -O0 -c %s -o %t.o
// RUN: llvm-nm --undefined-only %t.o | count 0
// RUN: %clang --target=mmix-unknown-unknown -std=c++17 -ffreestanding \
// RUN:   -fno-builtin -fno-exceptions -fno-rtti -fno-unwind-tables \
// RUN:   -fno-asynchronous-unwind-tables -fno-lax-vector-conversions \
// RUN:   -ftrivial-auto-var-init=pattern -DLIBC_FULL_BUILD \
// RUN:   -DLIBC_NAMESPACE=__llvm_libc -I %S/../../../libc -O3 -S %s -o - \
// RUN:   | FileCheck %s
// RUN: %clang --target=mmix-unknown-unknown -std=c++17 -ffreestanding \
// RUN:   -fno-builtin -fno-exceptions -fno-rtti -fno-unwind-tables \
// RUN:   -fno-asynchronous-unwind-tables -fno-lax-vector-conversions \
// RUN:   -ftrivial-auto-var-init=pattern -DLIBC_FULL_BUILD \
// RUN:   -DLIBC_NAMESPACE=__llvm_libc -I %S/../../../libc -O3 -c %s -o %t.opt.o
// RUN: llvm-nm --undefined-only %t.opt.o | count 0

#include "src/__support/CPP/functional.h"
#include "src/__support/CPP/simd.h"

namespace cpp = LIBC_NAMESPACE::cpp;

using U32x1 = cpp::fixed_size_simd<unsigned, 1>;
using U32x2 = cpp::fixed_size_simd<unsigned, 2>;
using F32x2 = cpp::fixed_size_simd<float, 2>;

// CHECK-LABEL: producer_integer:
// CHECK-NOT:   PUSHJ
// CHECK:       ADDU
// CHECK:       SUBU
// CHECK:       XOR
// CHECK:       POP 0, 0
extern "C" U32x2 producer_integer(U32x2 Lhs, U32x2 Rhs) {
  return (Lhs + Rhs) ^ (Lhs - Rhs);
}

// CHECK-LABEL: producer_float:
// CHECK-NOT:   PUSHJ
// CHECK:       FMUL
// CHECK:       FADD
// CHECK:       POP 0, 0
extern "C" F32x2 producer_float(F32x2 Lhs, F32x2 Rhs) {
  return Lhs * Rhs + Lhs;
}

// CHECK-LABEL: producer_convert:
// CHECK-NOT:   PUSHJ
// CHECK:       FLOTU
// CHECK:       POP 0, 0
extern "C" F32x2 producer_convert(U32x2 Value) {
  return cpp::simd_cast<float>(Value);
}

// CHECK-LABEL: producer_concat:
// CHECK-NOT:   PUSHJ
// CHECK:       SLU
// CHECK:       OR
// CHECK:       POP 0, 0
extern "C" U32x2 producer_concat(U32x1 Lhs, U32x1 Rhs) {
  return cpp::concat(Lhs, Rhs);
}

// CHECK-LABEL: producer_reduce:
// CHECK-NOT:   PUSHJ
// CHECK:       ADDU
// CHECK:       POP 0, 0
extern "C" unsigned producer_reduce(U32x2 Value) {
  return cpp::reduce(Value, cpp::plus<>{});
}

// CHECK-LABEL: producer_memory:
// CHECK-NOT:   PUSHJ
// CHECK:       LDOU
// CHECK:       STOU
// CHECK:       POP 0, 0
extern "C" void producer_memory(U32x2 *Dst, const U32x2 *Src) {
  U32x2 Value = cpp::load<U32x2>(Src, true);
  cpp::store(Value, Dst, true);
}
