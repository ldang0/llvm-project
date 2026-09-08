// REQUIRES: mmix-registered-target
// RUN: split-file %s %t
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti \
// RUN:   -mrelocation-model static -emit-obj -O2 -o %t.user.o %t/user.cpp
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c11 -ffreestanding \
// RUN:   -mrelocation-model static -emit-obj -O2 -o %t.dso.o \
// RUN:   %S/../../../compiler-rt/lib/builtins/mmix/crtdso.c
// RUN: llvm-readobj --symbols --relocations %t.dso.o \
// RUN:   | FileCheck %s --check-prefix=DSO-OBJECT
// RUN: llvm-ar rc %t.runtime.a %t.dso.o
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c11 -ffreestanding \
// RUN:   -mrelocation-model static -emit-obj -O2 -o %t.provider.o \
// RUN:   %t/provider.c
// RUN: ld.lld -m elf64mmix -r -o %t.linked.o %t.user.o %t.runtime.a \
// RUN:   %t.runtime.a %t.provider.o
// RUN: llvm-nm --undefined-only %t.linked.o | count 0
// RUN: llvm-readobj --symbols %t.linked.o \
// RUN:   | FileCheck %s --check-prefix=LINKED

//--- user.cpp
struct Value {
  ~Value();
};

extern "C" void observe(void *);
Value::~Value() { observe(this); }
Value value;

//--- provider.c
int __cxa_atexit(void (*callback)(void *), void *payload, void *dso) {
  (void)callback;
  (void)payload;
  (void)dso;
  return 0;
}

void observe(void *object) { (void)object; }

// DSO-OBJECT: R_MMIX_64 __dso_handle 0x0
// DSO-OBJECT: Name: __dso_handle
// DSO-OBJECT: Size: 8
// DSO-OBJECT: Binding: Global
// DSO-OBJECT: Type: Object
// DSO-OBJECT: Other [
// DSO-OBJECT-NEXT: STV_HIDDEN

// LINKED-COUNT-1: Name: __dso_handle
// LINKED: Size: 8
// LINKED: Binding: Global
// LINKED: Type: Object
// LINKED: Other [
// LINKED-NEXT: STV_HIDDEN
