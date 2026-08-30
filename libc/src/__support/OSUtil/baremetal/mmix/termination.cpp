//===-- MMIX bare-metal process termination ------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/__support/OSUtil/baremetal/mmix/semihosting.h"

using LIBC_NAMESPACE::internal::mmix::Semihosting;

extern "C" [[noreturn]] void __llvm_libc_exit(int status) {
  Semihosting::halt(static_cast<unsigned>(status));
}

// Override compiler-rt's weak standalone loop when LLVM libc owns the process.
extern "C" [[noreturn]] void __mmix_stack_chk_terminate() {
  Semihosting::halt(127);
}
