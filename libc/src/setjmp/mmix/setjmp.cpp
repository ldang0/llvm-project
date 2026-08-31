//===-- Implementation of setjmp for MMIX --------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/__support/common.h"
#include "src/setjmp/setjmp_impl.h"

#if !defined(__mmix__)
#error "Invalid file include"
#endif

#if defined(LIBC_COPT_PUBLIC_PACKAGING)

// POP exposes the register-stack offset that preceded the setjmp call. Global
// temporaries retain the buffer and return address while the local window is
// removed.
asm(R"(
  .section .text.setjmp,"ax",@progbits
  .global setjmp
  .type setjmp,@function
setjmp:
  OR r251, r231, 0
  STOU r253, r251, 0
  GET r252, rJ
  STOU r252, r251, 8
  STOU r254, r251, 16
  SETL r231, 0

  GETA r255, 0f
  PUT rJ, r255
  POP 0, 0
0:
  GET r255, rO
  STOU r255, r251, 24
  GO r255, r252, 0
  .size setjmp,.-setjmp
)");

#else

extern "C" [[gnu::returns_twice]] int __llvm_libc_mmix_setjmp(jmp_buf);

// Internal libc tests call a namespaced C++ entrypoint. Preserve its local r30
// across the otherwise identical assembly context switch.
asm(R"(
  .section .text.__llvm_libc_mmix_setjmp,"ax",@progbits
  .hidden __llvm_libc_mmix_setjmp
  .type __llvm_libc_mmix_setjmp,@function
__llvm_libc_mmix_setjmp:
  OR r251, r231, 0
  STOU r253, r251, 0
  GET r252, rJ
  STOU r252, r251, 8
  STOU r254, r251, 16
  SETL r231, 0

  GETA r255, 0f
  PUT rJ, r255
  POP 0, 0
0:
  GET r255, rO
  STOU r255, r251, 24
  STOU r30, r251, 32
  GO r255, r252, 0
  .size __llvm_libc_mmix_setjmp,.-__llvm_libc_mmix_setjmp
)");

namespace LIBC_NAMESPACE_DECL {

LLVM_LIBC_FUNCTION(int, setjmp, (jmp_buf buf)) {
  return __llvm_libc_mmix_setjmp(buf);
}

} // namespace LIBC_NAMESPACE_DECL

#endif
