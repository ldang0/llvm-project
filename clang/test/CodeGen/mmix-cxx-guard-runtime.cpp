// REQUIRES: mmix-registered-target
// RUN: split-file %s %t
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti \
// RUN:   -mrelocation-model static -emit-obj -O0 -o %t.user.o %t/user.cpp
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c11 -ffreestanding \
// RUN:   -mrelocation-model static -emit-obj -O2 -o %t.guard.o \
// RUN:   %S/../../../compiler-rt/lib/builtins/mmix/cxx_guard.c
// RUN: llvm-ar rc %t.runtime.a %t.guard.o
// RUN: llvm-nm --defined-only --extern-only %t.runtime.a \
// RUN:   | FileCheck %s --check-prefix=SYMBOL
// RUN: ld.lld -m elf64mmix -r -o %t.linked.o %t.user.o %t.runtime.a
// RUN: llvm-nm --undefined-only %t.linked.o \
// RUN:   | FileCheck %s --check-prefix=DEPENDENCY
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c11 -ffreestanding \
// RUN:   -mrelocation-model static -emit-llvm-bc -o %t.guard.bc \
// RUN:   %S/../../../compiler-rt/lib/builtins/mmix/cxx_guard.c
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c11 -ffreestanding \
// RUN:   -mrelocation-model static -emit-llvm-bc -o %t.harness.bc \
// RUN:   %t/guard-harness.c
// RUN: llvm-link %t.guard.bc %t.harness.bc -o %t.guard-test.bc
// RUN: lli -force-interpreter=true -entry-function=success %t.guard-test.bc
// RUN: not lli -force-interpreter=true -entry-function=recursive \
// RUN:   %t.guard-test.bc
// RUN: not lli -force-interpreter=true -entry-function=malformed \
// RUN:   %t.guard-test.bc
// RUN: not lli -force-interpreter=true -entry-function=invalid_release \
// RUN:   %t.guard-test.bc

//--- user.cpp
struct Value {
  long value;
  explicit Value(long input) : value(input) {}
};

static long make_value() { return 7; }

extern "C" long guarded_value() {
  static Value value(make_value());
  return value.value;
}

//--- guard-harness.c
typedef unsigned long long guard_type;

int __cxa_guard_acquire(guard_type *);
void __cxa_guard_release(guard_type *);

extern void exit(int) __attribute__((noreturn));
void abort(void) { exit(86); }

int success(void) {
  guard_type guard = 0;
  unsigned char *bytes = (unsigned char *)&guard;
  if (__cxa_guard_acquire(&guard) != 1)
    return 1;
  if (bytes[0] != 0 || bytes[1] != 2)
    return 2;
  __cxa_guard_release(&guard);
  if (bytes[0] != 1 || bytes[1] != 1)
    return 3;
  if (__cxa_guard_acquire(&guard) != 0)
    return 4;

  bytes[0] = 7;
  bytes[1] = 0;
  if (__cxa_guard_acquire(&guard) != 0)
    return 5;
  return 0;
}

int recursive(void) {
  guard_type guard = 0;
  ((unsigned char *)&guard)[1] = 2;
  return __cxa_guard_acquire(&guard);
}

int malformed(void) {
  guard_type guard = 0;
  ((unsigned char *)&guard)[1] = 7;
  return __cxa_guard_acquire(&guard);
}

int invalid_release(void) {
  guard_type guard = 0;
  __cxa_guard_release(&guard);
  return 0;
}

// SYMBOL-DAG: T __cxa_guard_acquire
// SYMBOL-DAG: T __cxa_guard_release
// SYMBOL-NOT: __cxa_guard_abort

// DEPENDENCY: U abort
// DEPENDENCY-NOT: __atomic
// DEPENDENCY-NOT: __cxa_guard
// DEPENDENCY-NOT: pthread
