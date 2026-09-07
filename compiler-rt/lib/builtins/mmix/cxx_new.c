//===-- cxx_new.c - MMIX C++ allocation runtime -------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

extern void *malloc(__SIZE_TYPE__);
extern void abort(void) __attribute__((noreturn));

#define MMIX_CXX_SYMBOL(name)                                                  \
  __attribute__((weak, visibility("default"))) __asm__(name)

static void *allocate(__SIZE_TYPE__ size) {
  void *result = malloc(size == 0 ? 1 : size);
  if (result == 0)
    abort();
  return result;
}

void *mmix_operator_new(__SIZE_TYPE__ size) MMIX_CXX_SYMBOL("_Znwm");
void *mmix_operator_new(__SIZE_TYPE__ size) { return allocate(size); }

void *mmix_operator_new_array(__SIZE_TYPE__ size) MMIX_CXX_SYMBOL("_Znam");
void *mmix_operator_new_array(__SIZE_TYPE__ size) { return allocate(size); }
