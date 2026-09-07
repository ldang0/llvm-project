//===-- cxx_delete.c - MMIX C++ deallocation runtime --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

extern void free(void *);

#define MMIX_CXX_SYMBOL(name)                                                  \
  __attribute__((weak, visibility("default"))) __asm__(name)

void mmix_operator_delete(void *pointer) MMIX_CXX_SYMBOL("_ZdlPv");
void mmix_operator_delete(void *pointer) { free(pointer); }

void mmix_operator_delete_array(void *pointer) MMIX_CXX_SYMBOL("_ZdaPv");
void mmix_operator_delete_array(void *pointer) { free(pointer); }

void mmix_operator_delete_sized(void *pointer, __SIZE_TYPE__ size)
    MMIX_CXX_SYMBOL("_ZdlPvm");
void mmix_operator_delete_sized(void *pointer, __SIZE_TYPE__ size) {
  (void)size;
  free(pointer);
}

void mmix_operator_delete_array_sized(void *pointer, __SIZE_TYPE__ size)
    MMIX_CXX_SYMBOL("_ZdaPvm");
void mmix_operator_delete_array_sized(void *pointer, __SIZE_TYPE__ size) {
  (void)size;
  free(pointer);
}
