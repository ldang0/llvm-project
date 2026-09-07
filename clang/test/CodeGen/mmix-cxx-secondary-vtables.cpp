// REQUIRES: mmix-registered-target
// RUN: split-file %s %t
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti -O0 \
// RUN:   -mrelocation-model static -emit-llvm -o %t/provider.ll %t/provider.cpp
// RUN: FileCheck %s --check-prefix=IR < %t/provider.ll
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti -O0 \
// RUN:   -mrelocation-model static -emit-obj -o %t/provider.o %t/provider.cpp
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti -O0 \
// RUN:   -mrelocation-model static -emit-obj -o %t/consumer.o %t/consumer.cpp
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti -O0 \
// RUN:   -mrelocation-model static -emit-obj -o %t/inline-a.o %t/inline-a.cpp
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti -O0 \
// RUN:   -mrelocation-model static -emit-obj -o %t/inline-b.o %t/inline-b.cpp
// RUN: llvm-readobj --sections --section-groups --symbols --relocations \
// RUN:   %t/provider.o | FileCheck %s --check-prefix=OBJECT
// RUN: llvm-nm %t/provider.o | FileCheck %s --check-prefix=THUNK-NM
// RUN: llvm-nm %t/inline-a.o %t/inline-b.o \
// RUN:   | FileCheck %s --check-prefix=DUPLICATE
// RUN: ld.lld -m elf64mmix -e c_entry -o %t/linked %t/provider.o \
// RUN:   %t/consumer.o %t/inline-a.o %t/inline-b.o
// RUN: llvm-nm --undefined-only %t/linked | count 0
// RUN: llvm-nm --defined-only %t/linked \
// RUN:   | FileCheck %s --check-prefix=LINKED
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti -O2 \
// RUN:   -mrelocation-model static -emit-obj -o %t/provider-opt.o \
// RUN:   %t/provider.cpp
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti -O2 \
// RUN:   -mrelocation-model static -emit-obj -o %t/consumer-opt.o \
// RUN:   %t/consumer.cpp
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti -O2 \
// RUN:   -mrelocation-model static -emit-obj -o %t/inline-a-opt.o \
// RUN:   %t/inline-a.cpp
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti -O2 \
// RUN:   -mrelocation-model static -emit-obj -o %t/inline-b-opt.o \
// RUN:   %t/inline-b.cpp
// RUN: ld.lld -m elf64mmix -e c_entry -o %t/linked-opt %t/provider-opt.o \
// RUN:   %t/consumer-opt.o %t/inline-a-opt.o %t/inline-b-opt.o
// RUN: llvm-nm --undefined-only %t/linked-opt | count 0

//--- secondary-vtables.h
struct Left {
  long left;
  explicit Left(long);
  virtual long value() const;
  virtual Left *self();
};

struct Right {
  long right;
  explicit Right(long);
  virtual long value() const;
  virtual Right *self();
};

struct Derived : Left, Right {
  long own;
  explicit Derived(long);
  long value() const override;
  Derived *self() override;
};

long call_left(Left *);
long call_right(Right *);
long inline_a(long);
long inline_b(long);
extern "C" long c_entry(long);

//--- inline-object.h
struct InlineObject {
  long value;
  explicit InlineObject(long input) : value(input) {}
  virtual long read() const { return value; }
};

//--- provider.cpp
#include "secondary-vtables.h"

Left::Left(long input) : left(input) {}
long Left::value() const { return left; }
Left *Left::self() { return this; }
Right::Right(long input) : right(input) {}
long Right::value() const { return right; }
Right *Right::self() { return this; }

Derived::Derived(long input) : Left(input), Right(input + 1), own(input + 2) {}
long Derived::value() const { return left + right + own; }
Derived *Derived::self() { return this; }

//--- consumer.cpp
#include "secondary-vtables.h"

__attribute__((noinline)) long call_left(Left *object) {
  return object->value() + object->self()->left;
}

__attribute__((noinline)) long call_right(Right *object) {
  return object->value() + object->self()->right;
}

extern "C" long c_entry(long input) {
  Derived object(input);
  return call_left(&object) + call_right(&object) + inline_a(input) +
         inline_b(input);
}

//--- inline-a.cpp
#include "inline-object.h"
long inline_a(long input) {
  InlineObject object(input);
  return object.read();
}

//--- inline-b.cpp
#include "inline-object.h"
long inline_b(long input) {
  InlineObject object(input + 1);
  return object.read();
}

// IR: @_ZTV7Derived ={{.*}} constant { [4 x ptr], [4 x ptr] }
// IR-LABEL: define dso_local noundef i64 @_ZThn16_NK7Derived5valueEv(ptr {{.*}}%this)
// IR: getelementptr inbounds i8, ptr %{{.*}}, i64 -16
// IR: tail call noundef i64 @_ZNK7Derived5valueEv(ptr {{.*}})
// IR-LABEL: define dso_local noundef ptr @_ZTchn16_h16_N7Derived4selfEv(ptr {{.*}}%this)
// IR: getelementptr inbounds i8, ptr %{{.*}}, i64 -16
// IR: call noundef ptr @_ZN7Derived4selfEv(ptr {{.*}})
// IR: getelementptr inbounds i8, ptr %{{.*}}, i64 16

// OBJECT: R_MMIX_64 _ZThn16_NK7Derived5valueEv 0x0
// OBJECT: R_MMIX_64 _ZTchn16_h16_N7Derived4selfEv 0x0
// OBJECT-DAG: Name: _ZTV7Derived
// OBJECT-DAG: Name: _ZThn16_NK7Derived5valueEv
// OBJECT-DAG: Name: _ZTchn16_h16_N7Derived4selfEv

// THUNK-NM-DAG: T _ZThn16_NK7Derived5valueEv
// THUNK-NM-DAG: T _ZTchn16_h16_N7Derived4selfEv
// DUPLICATE-COUNT-2: V _ZTV12InlineObject
// LINKED-COUNT-1: V _ZTV12InlineObject
// LINKED-DAG: T _ZThn16_NK7Derived5valueEv
// LINKED-DAG: T _ZTchn16_h16_N7Derived4selfEv
