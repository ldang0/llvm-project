// REQUIRES: mmix-registered-target
// RUN: split-file %s %t
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti \
// RUN:   -mrelocation-model static -emit-obj -O0 -o %t.user.o %t/user.cpp
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c11 -ffreestanding \
// RUN:   -mrelocation-model static -emit-obj -O2 -o %t.failure.o \
// RUN:   %S/../../../compiler-rt/lib/builtins/mmix/cxx_failure.c
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c11 -ffreestanding \
// RUN:   -mrelocation-model static -emit-obj -O2 -o %t.new.o \
// RUN:   %S/../../../compiler-rt/lib/builtins/mmix/cxx_new.c
// RUN: llvm-ar rc %t.runtime.a %t.failure.o %t.new.o
// RUN: llvm-nm --defined-only --extern-only %t.runtime.a \
// RUN:   | FileCheck %s --check-prefix=SYMBOL
// RUN: ld.lld -m elf64mmix -r -o %t.linked.o %t.user.o %t.runtime.a
// RUN: llvm-nm --undefined-only %t.linked.o \
// RUN:   | FileCheck %s --check-prefix=PURE-DEPENDENCY
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c11 -ffreestanding \
// RUN:   -mrelocation-model static -emit-llvm-bc -o %t.failure.bc \
// RUN:   %S/../../../compiler-rt/lib/builtins/mmix/cxx_failure.c
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c11 -ffreestanding \
// RUN:   -mrelocation-model static -emit-llvm-bc -o %t.new.bc \
// RUN:   %S/../../../compiler-rt/lib/builtins/mmix/cxx_new.c
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c11 -ffreestanding \
// RUN:   -mrelocation-model static -emit-llvm-bc -o %t.harness.bc \
// RUN:   %t/failure-harness.c
// RUN: llvm-link %t.failure.bc %t.new.bc %t.harness.bc -o %t.failure-test.bc
// RUN: not --crash lli -force-interpreter=true \
// RUN:   -entry-function=allocation_failure %t.failure-test.bc 2> /dev/null
// RUN: not --crash lli -force-interpreter=true \
// RUN:   -entry-function=pure_virtual_failure %t.failure-test.bc 2> /dev/null

//--- user.cpp
struct Abstract {
  virtual ~Abstract();
  virtual long value() const = 0;
};

Abstract::~Abstract() {}

//--- failure-harness.c
typedef __SIZE_TYPE__ size_t;

void *operator_new(size_t) __asm__("_Znwm");
void __cxa_pure_virtual(void);

void *malloc(size_t size) {
  (void)size;
  return 0;
}

void abort(void) { __builtin_trap(); }

int allocation_failure(void) {
  operator_new(8);
  return 0;
}

int pure_virtual_failure(void) {
  __cxa_pure_virtual();
  return 0;
}

// SYMBOL-DAG: W _Znwm
// SYMBOL-DAG: T __cxa_pure_virtual
// SYMBOL-NOT: __cxa_deleted_virtual

// PURE-DEPENDENCY: U abort
// PURE-DEPENDENCY-NOT: malloc
// PURE-DEPENDENCY-NOT: __cxa_pure_virtual
// PURE-DEPENDENCY-NOT: __cxa_deleted_virtual
// PURE-DEPENDENCY-NOT: __cxa_throw
// PURE-DEPENDENCY-NOT: _Unwind
// PURE-DEPENDENCY-NOT: new_handler
