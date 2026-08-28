// REQUIRES: mmix-registered-target
// RUN: %clang --target=mmix-unknown-unknown -std=c++17 -ffreestanding \
// RUN:   -fno-builtin -fno-exceptions -fno-rtti -fno-unwind-tables \
// RUN:   -fno-asynchronous-unwind-tables -fno-lax-vector-conversions \
// RUN:   -ftrivial-auto-var-init=pattern -fno-pic -fno-pie -nostdinc++ \
// RUN:   -DLIBC_FULL_BUILD \
// RUN:   -DLIBC_NAMESPACE=__llvm_libc -I %S/../../../libc -O0 -c %s -o %t.o
// RUN: llvm-readobj --file-headers --symbols %t.o | FileCheck %s

#include "src/string/memory_utils/utils.h"

extern "C" bool producer_is_disjoint(const void *Lhs, const void *Rhs,
                                      unsigned long Size) {
  return LIBC_NAMESPACE::is_disjoint(Lhs, Rhs, Size);
}

// CHECK: Format: elf64-mmix
// CHECK: Machine: EM_MMIX
// CHECK: Name: producer_is_disjoint
