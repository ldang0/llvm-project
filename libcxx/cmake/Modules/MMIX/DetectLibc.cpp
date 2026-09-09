//===-- Identify the selected installed MMIX C library ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include <errno.h>

#ifndef __mmix__
#error "The MMIX runtime cache requires an MMIX compiler target"
#endif

#if defined(__LLVM_LIBC__)
MMIX_LIBCXX_LLVM_LIBC
#elif defined(__NEWLIB__)
MMIX_LIBCXX_NEWLIB
#else
#error "The MMIX runtime cache requires installed LLVM libc or newlib headers"
#endif
