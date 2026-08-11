// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -E -dM -ffreestanding -fgnuc-version=4.2.1 \
// RUN:   -triple=mmix-unknown-unknown < /dev/null \
// RUN:   | FileCheck -match-full-lines --implicit-check-not=__FLT16 \
// RUN:       --implicit-check-not=__FLT128 --implicit-check-not=__BFLT16 %s

// CHECK: #define _LP64 1
// CHECK: #define __BIGGEST_ALIGNMENT__ 8
// CHECK: #define __BYTE_ORDER__ __ORDER_BIG_ENDIAN__
// CHECK: #define __CHAR_BIT__ 8
// CHECK: #define __DBL_MANT_DIG__ 53
// CHECK: #define __INT16_TYPE__ short
// CHECK: #define __INT32_TYPE__ int
// CHECK: #define __INT64_TYPE__ long int
// CHECK: #define __INTMAX_TYPE__ long int
// CHECK: #define __INTPTR_TYPE__ long int
// CHECK: #define __LDBL_MANT_DIG__ 53
// CHECK: #define __LONG_MAX__ 9223372036854775807L
// CHECK: #define __LP64__ 1
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
// CHECK: #define __UINT64_TYPE__ long unsigned int
// CHECK: #define __UINTMAX_TYPE__ long unsigned int
// CHECK: #define __UINTPTR_TYPE__ long unsigned int
// CHECK: #define __WCHAR_TYPE__ int
// CHECK: #define __WINT_TYPE__ unsigned int
