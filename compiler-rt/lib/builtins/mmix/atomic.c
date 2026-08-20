//===-- atomic.c - MMIX atomic fallback adaptation ------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include <stddef.h>

static void *mmix_atomic_memcpy(void *Dest, const void *Src, size_t Size) {
  unsigned char *To = Dest;
  const unsigned char *From = Src;
  for (size_t I = 0; I != Size; ++I)
    To[I] = From[I];
  return Dest;
}

static int mmix_atomic_memcmp(const void *LHS, const void *RHS, size_t Size) {
  const unsigned char *Left = LHS;
  const unsigned char *Right = RHS;
  for (size_t I = 0; I != Size; ++I) {
    if (Left[I] != Right[I])
      return Left[I] < Right[I] ? -1 : 1;
  }
  return 0;
}

// MMIX does not support i128 CodeGen. The generic implementation only uses
// it as an optional native-width fast path.
#undef __SIZEOF_INT128__
#define __builtin_memcpy mmix_atomic_memcpy
#define __builtin_memcmp mmix_atomic_memcmp

#include "../atomic.c"
