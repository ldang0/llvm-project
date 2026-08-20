//===-- stack_protector_guard.c - MMIX stack protector guard --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

typedef unsigned long long mmix_stack_guard_t;

_Static_assert(sizeof(mmix_stack_guard_t) == 8,
               "MMIX stack guards must occupy one octa");

__attribute__((weak, visibility("default"))) mmix_stack_guard_t
    __stack_chk_guard = 0x4d4d495853535031ULL;
