//===-- MMIX bare-metal standard streams ---------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "hdr/types/FILE.h"

// FIXME: Use the common C++ definitions once MMIX supports their global
// reinterpret_cast initialization without dynamic initialization.
extern char __llvm_libc_stdin_cookie;
extern char __llvm_libc_stdout_cookie;
extern char __llvm_libc_stderr_cookie;

FILE *stdin = (FILE *)&__llvm_libc_stdin_cookie;
FILE *stdout = (FILE *)&__llvm_libc_stdout_cookie;
FILE *stderr = (FILE *)&__llvm_libc_stderr_cookie;
