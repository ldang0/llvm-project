//===-- Implementation of longjmp for MMIX -------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/setjmp/longjmp.h"
#include "src/__support/common.h"

#if !defined(__mmix__)
#error "Invalid file include"
#endif

#if defined(LIBC_COPT_PUBLIC_PACKAGING)

// POP discards register-stack frames until the offset captured by setjmp is
// reached. The final transfer restores the software stack and saved return
// address. The fifth context word carries the requested return value while the
// register stack is unwound.
asm(R"(
  .section .text.longjmp,"ax",@progbits
  .global longjmp
  .type longjmp,@function
longjmp:
  CSZ r232, r232, 1
  STOU r232, r231, 32
  OR r251, r231, 0

  GETA r255, 0f
  PUT rJ, r255
  LDOU r255, r251, 24
0:
  GET r252, rO
  CMPU r252, r252, r255
  BNP r252, 1f
  LDOU r231, r251, 32
  POP 0, 0
1:
  LDOU r253, r251, 0
  LDOU r255, r251, 8
  LDOU r254, r251, 16
  GO r255, r255, 0
  .size longjmp,.-longjmp
)");

#else

extern "C" [[noreturn]] void __llvm_libc_mmix_longjmp(jmp_buf, int);

// The internal wrapper's r30 is stored in the fifth context word, so its
// return value stays in a global register while frames are discarded.
asm(R"(
  .section .text.__llvm_libc_mmix_longjmp,"ax",@progbits
  .hidden __llvm_libc_mmix_longjmp
  .type __llvm_libc_mmix_longjmp,@function
__llvm_libc_mmix_longjmp:
  OR r251, r231, 0
  OR r231, r232, 0
  CSZ r231, r232, 1

  GETA r255, 0f
  PUT rJ, r255
  LDOU r255, r251, 24
0:
  GET r252, rO
  CMPU r252, r252, r255
  BNP r252, 1f
  POP 0, 0
1:
  LDOU r30, r251, 32
  LDOU r253, r251, 0
  LDOU r255, r251, 8
  LDOU r254, r251, 16
  GO r255, r255, 0
  .size __llvm_libc_mmix_longjmp,.-__llvm_libc_mmix_longjmp
)");

namespace LIBC_NAMESPACE_DECL {

LLVM_LIBC_FUNCTION(void, longjmp, (jmp_buf buf, int val)) {
  __llvm_libc_mmix_longjmp(buf, val);
}

} // namespace LIBC_NAMESPACE_DECL

#endif
