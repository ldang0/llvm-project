// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti \
// RUN:   -mrelocation-model static -emit-obj -O0 -o %t.o %s
// RUN: llvm-nm --undefined-only %t.o | FileCheck %s

// Generic libc++abi execution is covered by the installed runtime workflow.
struct Value {
  long value;
  explicit Value(long input) : value(input) {}
};

static long make_value() { return 7; }

extern "C" long guarded_value() {
  static Value value(make_value());
  return value.value;
}

// CHECK-DAG: U __cxa_guard_acquire
// CHECK-DAG: U __cxa_guard_release
// CHECK-NOT: __cxa_guard_abort
