// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -E -dM -ffreestanding -fgnuc-version=4.2.1 \
// RUN:   -triple=mmix-unknown-unknown < /dev/null \
// RUN:   | FileCheck -match-full-lines --implicit-check-not=__FLT16 \
// RUN:       --implicit-check-not=__FLT128 --implicit-check-not=__BFLT16 \
// RUN:       --implicit-check-not=__MMIX_ABI_MMIXWARE__ \
// RUN:       --implicit-check-not=__STDC_IEC_ \
// RUN:       --implicit-check-not=__STDC_FENV_ \
// RUN:       --implicit-check-not=__FP_FAST_FMA %s

// CHECK: #define _LP64 1
// CHECK: #define __BIGGEST_ALIGNMENT__ 8
// CHECK: #define __BYTE_ORDER__ __ORDER_BIG_ENDIAN__
// CHECK: #define __CHAR_BIT__ 8
// CHECK: #define __DBL_HAS_DENORM__ 1
// CHECK: #define __DBL_HAS_INFINITY__ 1
// CHECK: #define __DBL_HAS_QUIET_NAN__ 1
// CHECK: #define __DBL_MANT_DIG__ 53
// CHECK: #define __ELF__ 1
// CHECK: #define __FLT_HAS_DENORM__ 1
// CHECK: #define __FLT_HAS_INFINITY__ 1
// CHECK: #define __FLT_HAS_QUIET_NAN__ 1
// CHECK: #define __FLT_MANT_DIG__ 24
// CHECK: #define __FLT_RADIX__ 2
// CHECK: #define __GCC_ATOMIC_BOOL_LOCK_FREE 1
// CHECK: #define __GCC_ATOMIC_CHAR16_T_LOCK_FREE 1
// CHECK: #define __GCC_ATOMIC_CHAR32_T_LOCK_FREE 1
// CHECK: #define __GCC_ATOMIC_CHAR_LOCK_FREE 1
// CHECK: #define __GCC_ATOMIC_INT_LOCK_FREE 1
// CHECK: #define __GCC_ATOMIC_LLONG_LOCK_FREE 1
// CHECK: #define __GCC_ATOMIC_LONG_LOCK_FREE 1
// CHECK: #define __GCC_ATOMIC_POINTER_LOCK_FREE 1
// CHECK: #define __GCC_ATOMIC_SHORT_LOCK_FREE 1
// CHECK: #define __GCC_ATOMIC_WCHAR_T_LOCK_FREE 1
// CHECK: #define __INT16_TYPE__ short
// CHECK: #define __INT32_TYPE__ int
// CHECK: #define __INT64_TYPE__ long int
// CHECK: #define __INTMAX_TYPE__ long int
// CHECK: #define __INTPTR_TYPE__ long int
// CHECK: #define __LDBL_HAS_DENORM__ 1
// CHECK: #define __LDBL_HAS_INFINITY__ 1
// CHECK: #define __LDBL_HAS_QUIET_NAN__ 1
// CHECK: #define __LDBL_MANT_DIG__ 53
// CHECK: #define __LONG_MAX__ 9223372036854775807L
// CHECK: #define __LP64__ 1
// CHECK: #define __MMIX_ABI_GNU__ 1
// CHECK: #define __MMIX__ 1
// CHECK: #define __POINTER_WIDTH__ 64
// CHECK: #define __PTRDIFF_TYPE__ long int
// CHECK: #define __SIZEOF_DOUBLE__ 8
// CHECK: #define __SIZEOF_FLOAT__ 4
// CHECK: #define __SIZEOF_INT128__ 16
// CHECK: #define __SIZEOF_INT__ 4
// CHECK: #define __SIZEOF_LONG_DOUBLE__ 8
// CHECK: #define __SIZEOF_LONG_LONG__ 8
// CHECK: #define __SIZEOF_LONG__ 8
// CHECK: #define __SIZEOF_POINTER__ 8
// CHECK: #define __SIZEOF_SHORT__ 2
// CHECK: #define __SIZE_TYPE__ long unsigned int
// CHECK: #define __STDC_HOSTED__ 0
// CHECK: #define __UINT64_TYPE__ long unsigned int
// CHECK: #define __UINTMAX_TYPE__ long unsigned int
// CHECK: #define __UINTPTR_TYPE__ long unsigned int
// CHECK: #define __WCHAR_TYPE__ int
// CHECK: #define __WINT_TYPE__ unsigned int
// CHECK: #define __mmix__ 1
