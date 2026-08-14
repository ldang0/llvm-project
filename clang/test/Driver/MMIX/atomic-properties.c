// RUN: %clang --target=mmix-unknown-unknown -std=c17 -dM -E %s \
// RUN:   | FileCheck %s --check-prefix=MACROS
// RUN: %clang --target=mmix-unknown-unknown -std=c17 -fsyntax-only \
// RUN:   -Xclang -verify %s

// MACROS-DAG: #define __CLANG_ATOMIC_BOOL_LOCK_FREE 2
// MACROS-DAG: #define __CLANG_ATOMIC_CHAR_LOCK_FREE 2
// MACROS-DAG: #define __CLANG_ATOMIC_CHAR16_T_LOCK_FREE 2
// MACROS-DAG: #define __CLANG_ATOMIC_CHAR32_T_LOCK_FREE 2
// MACROS-DAG: #define __CLANG_ATOMIC_INT_LOCK_FREE 2
// MACROS-DAG: #define __CLANG_ATOMIC_LLONG_LOCK_FREE 2
// MACROS-DAG: #define __CLANG_ATOMIC_LONG_LOCK_FREE 2
// MACROS-DAG: #define __CLANG_ATOMIC_POINTER_LOCK_FREE 2
// MACROS-DAG: #define __CLANG_ATOMIC_SHORT_LOCK_FREE 2
// MACROS-DAG: #define __CLANG_ATOMIC_WCHAR_T_LOCK_FREE 2
// MACROS-DAG: #define __GCC_ATOMIC_BOOL_LOCK_FREE 2
// MACROS-DAG: #define __GCC_ATOMIC_CHAR_LOCK_FREE 2
// MACROS-DAG: #define __GCC_ATOMIC_CHAR16_T_LOCK_FREE 2
// MACROS-DAG: #define __GCC_ATOMIC_CHAR32_T_LOCK_FREE 2
// MACROS-DAG: #define __GCC_ATOMIC_INT_LOCK_FREE 2
// MACROS-DAG: #define __GCC_ATOMIC_LLONG_LOCK_FREE 2
// MACROS-DAG: #define __GCC_ATOMIC_LONG_LOCK_FREE 2
// MACROS-DAG: #define __GCC_ATOMIC_POINTER_LOCK_FREE 2
// MACROS-DAG: #define __GCC_ATOMIC_SHORT_LOCK_FREE 2
// MACROS-DAG: #define __GCC_ATOMIC_WCHAR_T_LOCK_FREE 2

// expected-no-diagnostics

struct ThreeBytes {
  unsigned char bytes[3];
};

struct FiveBytes {
  unsigned char bytes[5];
};

_Static_assert(sizeof(_Atomic(_Bool)) == 1, "");
_Static_assert(_Alignof(_Atomic(_Bool)) == 1, "");
_Static_assert(sizeof(_Atomic(short)) == 2, "");
_Static_assert(_Alignof(_Atomic(short)) == 2, "");
_Static_assert(sizeof(_Atomic(int)) == 4, "");
_Static_assert(_Alignof(_Atomic(int)) == 4, "");
_Static_assert(sizeof(_Atomic(long)) == 8, "");
_Static_assert(_Alignof(_Atomic(long)) == 8, "");
_Static_assert(sizeof(_Atomic(void *)) == 8, "");
_Static_assert(_Alignof(_Atomic(void *)) == 8, "");

_Static_assert(sizeof(_Atomic(struct ThreeBytes)) == 3, "");
_Static_assert(_Alignof(_Atomic(struct ThreeBytes)) == 1, "");
_Static_assert(sizeof(_Atomic(struct FiveBytes)) == 5, "");
_Static_assert(_Alignof(_Atomic(struct FiveBytes)) == 1, "");

_Static_assert(__atomic_always_lock_free(1, 0), "");
_Static_assert(__atomic_always_lock_free(2, 0), "");
_Static_assert(!__atomic_always_lock_free(3, 0), "");
_Static_assert(__atomic_always_lock_free(4, 0), "");
_Static_assert(!__atomic_always_lock_free(5, 0), "");
_Static_assert(__atomic_always_lock_free(8, 0), "");
_Static_assert(!__atomic_always_lock_free(16, 0), "");

_Static_assert(__atomic_always_lock_free(4, (void *)4), "");
_Static_assert(!__atomic_always_lock_free(4, (void *)2), "");
_Static_assert(__atomic_always_lock_free(8, (void *)8), "");
_Static_assert(!__atomic_always_lock_free(8, (void *)4), "");
