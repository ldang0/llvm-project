//===-- stack_protector_fail.c - MMIX stack protector failure ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

__attribute__((weak, visibility("default"), noreturn)) void
__mmix_stack_chk_terminate(void) {
  for (;;)
    __asm__ volatile("SWYM 0, 0, 0");
}

__attribute__((visibility("default"), noreturn)) void __stack_chk_fail(void) {
  __mmix_stack_chk_terminate();
}
