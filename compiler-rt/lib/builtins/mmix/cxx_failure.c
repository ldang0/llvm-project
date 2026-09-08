//===-- cxx_failure.c - MMIX C++ runtime failures -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

extern void abort(void) __attribute__((noreturn));

__attribute__((noreturn, visibility("default"))) void __cxa_pure_virtual(void) {
  abort();
}
