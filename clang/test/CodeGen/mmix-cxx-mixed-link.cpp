// REQUIRES: mmix-registered-target
// RUN: split-file %s %t
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c17 -O0 \
// RUN:   -mrelocation-model static -emit-obj -o %t/c-entry.o %t/c-entry.c
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O0 \
// RUN:   -mrelocation-model static -emit-obj -o %t/records.o %t/records.cpp
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O0 \
// RUN:   -mrelocation-model static -S -o %t/producer.s %t/producer.cpp
// RUN: FileCheck %s --check-prefix=ASM < %t/producer.s
// RUN: llvm-mc -triple=mmix -filetype=obj -o %t/producer.o %t/producer.s
// RUN: llvm-readobj --sections --section-groups --symbols --relocations \
// RUN:   %t/producer.o | FileCheck %s --check-prefix=OBJECT
// RUN: ld.lld -m elf64mmix -e c_entry -o %t/linked \
// RUN:   %t/c-entry.o %t/records.o %t/producer.o
// RUN: llvm-nm --undefined-only %t/linked | count 0
// RUN: llvm-readobj --sections --symbols --relocations %t/linked \
// RUN:   | FileCheck %s --check-prefix=LINKED \
// RUN:   --implicit-check-not=.init_array --implicit-check-not=.eh_frame \
// RUN:   --implicit-check-not=.gcc_except_table --implicit-check-not=.dynamic
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c17 -O2 \
// RUN:   -mrelocation-model static -emit-obj -o %t/c-entry-opt.o %t/c-entry.c
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O2 \
// RUN:   -mrelocation-model static -emit-obj -o %t/records-opt.o %t/records.cpp
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O2 \
// RUN:   -mrelocation-model static -emit-obj -o %t/producer-opt.o \
// RUN:   %t/producer.cpp
// RUN: ld.lld -m elf64mmix -e c_entry -o %t/linked-opt \
// RUN:   %t/c-entry-opt.o %t/records-opt.o %t/producer-opt.o
// RUN: llvm-nm --undefined-only %t/linked-opt | count 0
// RUN: llvm-readobj --sections --relocations %t/linked-opt \
// RUN:   | FileCheck %s --check-prefix=OPT-LINKED \
// RUN:   --implicit-check-not=.init_array --implicit-check-not=.eh_frame \
// RUN:   --implicit-check-not=.gcc_except_table --implicit-check-not=.dynamic

//--- common.h
#ifndef COMMON_H
#define COMMON_H

struct Trivial {
  long Value;
};

#ifdef __cplusplus
extern "C" {
#endif

long c_helper(long Value);
long cpp_entry(long Value);

#ifdef __cplusplus
}
#endif

#endif

//--- records.h
#include "common.h"

struct Managed {
  long Value;

  explicit Managed(long Value);
  Managed(const Managed &Other);
  Managed(Managed &&Other);
  ~Managed();

  long add(long Delta) const;
};

Managed transform(Managed Input, struct Trivial Delta);

template <typename T>
__attribute__((noinline)) T twice(T Value) {
  return Value + Value;
}

inline __attribute__((noinline)) long inline_adjust(long Value) {
  return Value + 3;
}

__attribute__((weak)) long weak_adjust(long Value);

//--- c-entry.c
#include "common.h"

long c_helper(long Value) { return Value + 5; }

long c_entry(long Value) { return cpp_entry(Value) + c_helper(Value); }

//--- records.cpp
#include "records.h"

Managed::Managed(long Value) : Value(Value) {}
Managed::Managed(const Managed &Other) : Value(Other.Value) {}
Managed::Managed(Managed &&Other) : Value(Other.Value + 1) {}
Managed::~Managed() { c_helper(Value); }

long Managed::add(long Delta) const { return Value + Delta; }

Managed transform(Managed Input, struct Trivial Delta) {
  Input.Value = Input.add(Delta.Value);
  return static_cast<Managed &&>(Input);
}

long weak_adjust(long Value) { return Value + 7; }

//--- producer.cpp
#include "records.h"

extern "C" long cpp_entry(long Value) {
  Managed Input(Value);
  Managed Output = transform(Input, Trivial{2});
  return c_helper(Output.Value) + twice(Output.Value) +
         inline_adjust(Output.Value) + weak_adjust(Output.Value);
}

// ASM:      .globl cpp_entry
// ASM:      cpp_entry:
// ASM:      GETA [[CALL:r[0-9]+]], %geta(_ZN7ManagedC1El)
// ASM:      PUSHGO r31, [[CALL]], 0
// ASM:      GETA [[TRANSFORM:r[0-9]+]], %geta(_Z9transform7Managed7Trivial)
// ASM:      PUSHGO r31, [[TRANSFORM]], 0
// ASM:      GETA [[C_HELPER:r[0-9]+]], %geta(c_helper)
// ASM:      PUSHGO r31, [[C_HELPER]], 0

// OBJECT:      Name: .group
// OBJECT:      Type: SHT_GROUP
// OBJECT:      Name: .text._Z5twiceIlET_S0_
// OBJECT:      SHF_GROUP
// OBJECT:      Name: .text._Z13inline_adjustl
// OBJECT:      SHF_GROUP
// OBJECT:      R_MMIX_GETA _ZN7ManagedC1El
// OBJECT:      R_MMIX_GETA _Z9transform7Managed7Trivial
// OBJECT:      R_MMIX_GETA c_helper
// OBJECT:      R_MMIX_GETA _Z5twiceIlET_S0_
// OBJECT:      R_MMIX_GETA _Z13inline_adjustl
// OBJECT:      R_MMIX_GETA _Z11weak_adjustl
// OBJECT:      Name: _Z5twiceIlET_S0_
// OBJECT:      Binding: Weak
// OBJECT:      Name: _Z13inline_adjustl
// OBJECT:      Binding: Weak

// LINKED:      Name: .text
// LINKED:      Relocations [
// LINKED-NEXT: ]
// LINKED:      Name: c_entry
// LINKED:      Name: cpp_entry
// LINKED:      Name: _Z9transform7Managed7Trivial
// LINKED:      Name: _Z11weak_adjustl
// LINKED:      Binding: Weak

// OPT-LINKED:      Name: .text
// OPT-LINKED:      Relocations [
// OPT-LINKED-NEXT: ]
