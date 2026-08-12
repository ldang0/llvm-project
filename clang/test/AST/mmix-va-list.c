// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c11 -fsyntax-only \
// RUN:   -verify %s
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c11 -ast-dump \
// RUN:   -ast-dump-filter mmix_variadic %s | FileCheck %s

// expected-no-diagnostics

#define SAME_TYPE(T, U) __builtin_types_compatible_p(T, U)

_Static_assert(SAME_TYPE(__builtin_va_list, void *),
               "va_list is one void pointer");
_Static_assert(!SAME_TYPE(__builtin_va_list, char *),
               "va_list is not a generic byte cursor type");
_Static_assert(sizeof(__builtin_va_list) == 8, "va_list size");
_Static_assert(_Alignof(__builtin_va_list) == 8, "va_list alignment");

struct DefaultAligned {
  char value;
} __attribute__((aligned));

_Static_assert(sizeof(struct DefaultAligned) == 8,
               "default aligned object size");
_Static_assert(_Alignof(struct DefaultAligned) == 8,
               "default aligned object alignment");

void mmix_variadic(int named, ...) {
  __builtin_va_list ap;
  __builtin_va_list copy;

  __builtin_va_start(ap, named);
  __builtin_va_copy(copy, ap);
  __builtin_va_end(copy);
  __builtin_va_end(ap);
}

// CHECK: FunctionDecl {{.*}} mmix_variadic 'void (int, ...)'
// CHECK: VarDecl {{.*}} ap '__builtin_va_list':'void *'
// CHECK: VarDecl {{.*}} copy '__builtin_va_list':'void *'
