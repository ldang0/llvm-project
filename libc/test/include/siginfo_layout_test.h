//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "../../include/llvm-libc-types/siginfo_t.h"

#ifdef __cplusplus
#define ASSERT(C) static_assert(C, #C)
#define ALIGNOF(T) alignof(T)
#define SAME_TYPE(A, B) __is_same(A, B)
#else
#define ASSERT(C) _Static_assert(C, #C)
#define ALIGNOF(T) _Alignof(T)
#define SAME_TYPE(A, B) __builtin_types_compatible_p(A, B)
#endif
#define OFFSET(F, N) ASSERT(__builtin_offsetof(siginfo_t, F) == (N))
#define TYPE(F, T) ASSERT(SAME_TYPE(__typeof__(((siginfo_t *)0)->F), T))

#ifdef __linux__
ASSERT(sizeof(siginfo_t) == 128);
#else
// Preserve the existing unknown-OS ABI rather than silently changing it.
ASSERT(sizeof(siginfo_t) == 144);
#endif

ASSERT(ALIGNOF(siginfo_t) == __SIZEOF_POINTER__);
OFFSET(si_signo, 0);
OFFSET(si_errno, 4);
OFFSET(si_code, 8);
#if __SIZEOF_POINTER__ == 8
#define PAYLOAD 16
OFFSET(si_utime, 32);
OFFSET(si_stime, 40);
OFFSET(si_addr_lsb, 24);
OFFSET(si_lower, 32);
OFFSET(si_upper, 40);
OFFSET(si_pkey, 32);
#else
#define PAYLOAD 12
OFFSET(si_utime, 24);
OFFSET(si_stime, 28);
OFFSET(si_addr_lsb, 16);
OFFSET(si_lower, 20);
OFFSET(si_upper, 24);
OFFSET(si_pkey, 20);
#endif
OFFSET(si_pid, PAYLOAD);
OFFSET(si_uid, PAYLOAD + 4);
OFFSET(si_timerid, PAYLOAD);
OFFSET(si_overrun, PAYLOAD + 4);
OFFSET(si_status, PAYLOAD + 8);
OFFSET(si_value, PAYLOAD + 8);
OFFSET(si_int, PAYLOAD + 8);
OFFSET(si_ptr, PAYLOAD + 8);
OFFSET(si_addr, PAYLOAD);
OFFSET(si_band, PAYLOAD);
OFFSET(si_fd, PAYLOAD + __SIZEOF_LONG__);
OFFSET(si_call_addr, PAYLOAD);
OFFSET(si_syscall, PAYLOAD + __SIZEOF_POINTER__);
OFFSET(si_arch, PAYLOAD + __SIZEOF_POINTER__ + 4);

TYPE(si_signo, int);
TYPE(si_errno, int);
TYPE(si_code, int);
TYPE(si_pid, int);
TYPE(si_uid, unsigned int);
TYPE(si_timerid, int);
TYPE(si_overrun, int);
TYPE(si_status, int);
TYPE(si_utime, long);
TYPE(si_stime, long);
TYPE(si_value, union sigval);
TYPE(si_int, int);
TYPE(si_ptr, void *);
TYPE(si_addr, void *);
TYPE(si_addr_lsb, short);
TYPE(si_lower, void *);
TYPE(si_upper, void *);
TYPE(si_pkey, unsigned int);
TYPE(si_band, long);
TYPE(si_fd, int);
TYPE(si_call_addr, void *);
TYPE(si_syscall, int);
TYPE(si_arch, unsigned int);

#if defined(SI_MAX_SIZE) || defined(SI_PREAMBLE_SIZE)
#error "internal size macro leaked"
#endif
