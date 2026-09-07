//===-- cxx_guard.c - MMIX single-threaded C++ guards -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

extern void abort(void) __attribute__((noreturn));

typedef unsigned long long mmix_cxx_guard_t;

_Static_assert(sizeof(mmix_cxx_guard_t) == 8,
               "MMIX C++ guards must occupy one octa");

enum {
  MMIX_GUARD_UNSET = 0,
  MMIX_GUARD_COMPLETE = 1,
  MMIX_GUARD_PENDING = 2,
};

static void fail_guard(void) { abort(); }

__attribute__((visibility("default"))) int
__cxa_guard_acquire(mmix_cxx_guard_t *guard) {
  if (guard == 0)
    fail_guard();

  unsigned char *bytes = (unsigned char *)guard;
  if (bytes[0] != MMIX_GUARD_UNSET)
    return 0;

  if (bytes[1] == MMIX_GUARD_PENDING)
    fail_guard();
  if (bytes[1] != MMIX_GUARD_UNSET)
    fail_guard();

  bytes[1] = MMIX_GUARD_PENDING;
  return 1;
}

__attribute__((visibility("default"))) void
__cxa_guard_release(mmix_cxx_guard_t *guard) {
  if (guard == 0)
    fail_guard();

  unsigned char *bytes = (unsigned char *)guard;
  if (bytes[0] != MMIX_GUARD_UNSET || bytes[1] != MMIX_GUARD_PENDING)
    fail_guard();

  bytes[0] = MMIX_GUARD_COMPLETE;
  bytes[1] = MMIX_GUARD_COMPLETE;
}
