// REQUIRES: mmix-registered-target
// RUN: split-file %s %t
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti \
// RUN:   -O0 -mrelocation-model static -emit-llvm -o - \
// RUN:   %t/provider.cpp | FileCheck %s --check-prefix=IR
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti \
// RUN:   -O0 -mrelocation-model static -emit-obj \
// RUN:   -o %t/provider.o %t/provider.cpp
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti \
// RUN:   -O0 -mrelocation-model static -emit-obj \
// RUN:   -o %t/consumer.o %t/consumer.cpp
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti \
// RUN:   -O0 -mrelocation-model static -emit-obj \
// RUN:   -o %t/abstract.o %t/abstract.cpp
// RUN: llvm-readobj --sections --symbols --relocations %t/provider.o \
// RUN:   | FileCheck %s --check-prefix=OBJECT
// RUN: llvm-nm %t/provider.o | FileCheck %s --check-prefix=NM
// RUN: llvm-readobj --symbols --relocations %t/abstract.o \
// RUN:   | FileCheck %s --check-prefix=ABSTRACT
// RUN: llvm-nm --undefined-only %t/abstract.o \
// RUN:   | FileCheck %s --check-prefix=ABSTRACT-NM
// RUN: ld.lld -m elf64mmix -e c_entry -o %t/linked \
// RUN:   %t/provider.o %t/consumer.o
// RUN: llvm-nm --undefined-only %t/linked | count 0
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti \
// RUN:   -O2 -mrelocation-model static -emit-obj \
// RUN:   -o %t/provider-opt.o %t/provider.cpp
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti \
// RUN:   -O2 -mrelocation-model static -emit-obj \
// RUN:   -o %t/consumer-opt.o %t/consumer.cpp
// RUN: ld.lld -m elf64mmix -e c_entry -o %t/linked-opt \
// RUN:   %t/provider-opt.o %t/consumer-opt.o
// RUN: llvm-nm --undefined-only %t/linked-opt | count 0

//--- primary-vtables.h
struct Base {
  long value;
  explicit Base(long);
  virtual long first(long) const;
  virtual long second(long) const;
};

struct Derived : Base {
  explicit Derived(long);
  long first(long) const override;
  long second(long) const override;
};

using BaseMember = long (Base::*)(long) const;
using DerivedMember = long (Derived::*)(long) const;

extern BaseMember base_virtual_member;
extern DerivedMember derived_virtual_member;
long dispatch(Base *, long);
extern "C" long c_entry(long);

//--- provider.cpp
#include "primary-vtables.h"

BaseMember base_virtual_member = &Base::first;
DerivedMember derived_virtual_member = &Derived::second;

Base::Base(long input) : value(input) {}
long Base::first(long input) const { return value + input; }
long Base::second(long input) const { return value - input; }
Derived::Derived(long input) : Base(input) {}
long Derived::first(long input) const { return value + input + 1; }
long Derived::second(long input) const { return value - input - 1; }

__attribute__((noinline)) long dispatch(Base *object, long input) {
  return object->first(input) + object->second(input);
}

//--- consumer.cpp
#include "primary-vtables.h"

extern "C" long c_entry(long input) {
  Derived object(input);
  return dispatch(&object, 3) + (object.*derived_virtual_member)(4);
}

//--- abstract.cpp
struct Abstract {
  virtual long anchor(long) const;
  virtual long pure(long) const = 0;
};

long Abstract::anchor(long input) const { return input; }

// IR: @base_virtual_member ={{.*}} global { i64, i64 } { i64 1, i64 0 }, align 8
// IR: @derived_virtual_member ={{.*}} global { i64, i64 } { i64 9, i64 0 }, align 8
// IR: @_ZTV4Base ={{.*}} constant { [4 x ptr] } { [4 x ptr] [ptr null, ptr null, ptr @_ZNK4Base5firstEl, ptr @_ZNK4Base6secondEl] }, align 8
// IR: @_ZTV7Derived ={{.*}} constant { [4 x ptr] } { [4 x ptr] [ptr null, ptr null, ptr @_ZNK7Derived5firstEl, ptr @_ZNK7Derived6secondEl] }, align 8
// IR-LABEL: define dso_local void @_ZN4BaseC2El(ptr {{.*}}%this, i64 {{.*}}%input)
// IR: store ptr getelementptr inbounds inrange(-16, 16) ({ [4 x ptr] }, ptr @_ZTV4Base, i32 0, i32 0, i32 2), ptr %{{.*}}, align 8
// IR-LABEL: define dso_local noundef i64 @_Z8dispatchP4Basel(ptr {{.*}}%object, i64 {{.*}}%input)
// IR: load ptr, ptr %{{.*}}, align 8
// IR: load ptr, ptr %{{.*}}, align 8
// IR: call noundef i64 %{{.*}}(ptr {{.*}}, i64 {{.*}})
// IR: getelementptr inbounds ptr, ptr %{{.*}}, i64 1
// IR: load ptr, ptr %{{.*}}, align 8
// IR: call noundef i64 %{{.*}}(ptr {{.*}}, i64 {{.*}})

// OBJECT: Name: .rodata
// OBJECT: AddressAlignment: 8
// OBJECT: R_MMIX_64 _ZNK4Base5firstEl 0x0
// OBJECT: R_MMIX_64 _ZNK4Base6secondEl 0x0
// OBJECT: R_MMIX_64 _ZNK7Derived5firstEl 0x0
// OBJECT: R_MMIX_64 _ZNK7Derived6secondEl 0x0
// OBJECT-DAG: Name: _ZTV4Base
// OBJECT-DAG: Name: _ZTV7Derived
// OBJECT-DAG: Name: _ZNK4Base5firstEl
// OBJECT-DAG: Name: _ZNK7Derived5firstEl

// NM-DAG: R _ZTV4Base
// NM-DAG: R _ZTV7Derived

// ABSTRACT: R_MMIX_64 __cxa_pure_virtual 0x0
// ABSTRACT-DAG: Name: _ZTV8Abstract
// ABSTRACT-DAG: Name: __cxa_pure_virtual
// ABSTRACT-DAG: Section: Undefined
// ABSTRACT-NM: U __cxa_pure_virtual
