// REQUIRES: mmix-registered-target
// RUN: split-file %s %t
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti -O0 \
// RUN:   -mrelocation-model static -emit-llvm -o %t/source-a.ll %t/source-a.cpp
// RUN: FileCheck %s --check-prefix=IR < %t/source-a.ll
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti -O0 \
// RUN:   -mrelocation-model static -emit-obj -o %t/source-a.o %t/source-a.cpp
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti -O0 \
// RUN:   -mrelocation-model static -emit-obj -o %t/source-b.o %t/source-b.cpp
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -O0 \
// RUN:   -mrelocation-model static -emit-obj -o %t/runtime-stubs.o \
// RUN:   %t/runtime-stubs.cpp
// RUN: llvm-readobj --sections --symbols --relocations %t/source-a.o \
// RUN:   | FileCheck %s --check-prefix=OBJECT
// RUN: ld.lld -m elf64mmix -e c_entry -Map=%t.map -o %t/linked \
// RUN:   %t/source-a.o %t/source-b.o %t/runtime-stubs.o
// RUN: FileCheck %s --check-prefix=MAP < %t.map
// RUN: llvm-nm --undefined-only %t/linked | count 0
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti -O2 \
// RUN:   -mrelocation-model static -emit-obj -o %t/source-a-opt.o \
// RUN:   %t/source-a.cpp
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti -O2 \
// RUN:   -mrelocation-model static -emit-obj -o %t/source-b-opt.o \
// RUN:   %t/source-b.cpp
// RUN: ld.lld -m elf64mmix -e c_entry -Map=%t-opt.map -o %t/linked-opt \
// RUN:   %t/source-a-opt.o %t/source-b-opt.o %t/runtime-stubs.o
// RUN: FileCheck %s --check-prefix=MAP-OPT < %t-opt.map
// RUN: llvm-nm --undefined-only %t/linked-opt | count 0

//--- initialization.h
extern long make_value(long);

struct Value {
  long value;
  explicit Value(long input) : value(input) {}
};

//--- source-a.cpp
#include "initialization.h"

__attribute__((init_priority(101))) Value early(make_value(1));
__attribute__((init_priority(200))) Value first(make_value(2));
__attribute__((init_priority(200))) Value second(make_value(3));
long scalar = make_value(4);

//--- source-b.cpp
#include "initialization.h"

__attribute__((init_priority(150))) Value middle(make_value(5));
Value last(make_value(6));

//--- runtime-stubs.cpp
long make_value(long value) { return value; }
extern "C" long c_entry() { return 0; }

// IR: @llvm.global_ctors = appending global [3 x { i32, ptr, ptr }]
// IR-SAME: i32 101, ptr @_GLOBAL__I_000101
// IR-SAME: i32 200, ptr @_GLOBAL__I_000200
// IR-SAME: i32 65535, ptr @_GLOBAL__sub_I_source_a.cpp

// IR-LABEL: define internal void @__cxx_global_var_init.1()
// IR: call noundef i64 @_Z10make_valuel(i64 noundef 2)
// IR: call void @_ZN5ValueC1El({{.*}}@first,
// IR-LABEL: define internal void @__cxx_global_var_init.2()
// IR: call noundef i64 @_Z10make_valuel(i64 noundef 3)
// IR: call void @_ZN5ValueC1El({{.*}}@second,
// IR-LABEL: define internal void @__cxx_global_var_init.3()
// IR: call noundef i64 @_Z10make_valuel(i64 noundef 4)
// IR: store i64 %{{.*}}, ptr @scalar, align 8

// OBJECT: Name: .init_array.101
// OBJECT: Type: SHT_INIT_ARRAY
// OBJECT: AddressAlignment: 8
// OBJECT: Name: .init_array.200
// OBJECT: Type: SHT_INIT_ARRAY
// OBJECT: AddressAlignment: 8
// OBJECT: Name: .init_array
// OBJECT: Type: SHT_INIT_ARRAY
// OBJECT: AddressAlignment: 8
// OBJECT: Section {{.*}} .rela.init_array.101
// OBJECT: R_MMIX_64 .text
// OBJECT: Section {{.*}} .rela.init_array.200
// OBJECT: R_MMIX_64 .text
// OBJECT: Section {{.*}} .rela.init_array
// OBJECT: R_MMIX_64 .text
// OBJECT-DAG: Name: _GLOBAL__sub_I_source_a.cpp

// MAP: .init_array
// MAP: {{.*}}source-a.o:(.init_array.101)
// MAP-NEXT: {{.*}}source-b.o:(.init_array.150)
// MAP-NEXT: {{.*}}source-a.o:(.init_array.200)
// MAP-OPT: .init_array
// MAP-OPT: {{.*}}source-a-opt.o:(.init_array.101)
// MAP-OPT-NEXT: {{.*}}source-b-opt.o:(.init_array.150)
// MAP-OPT-NEXT: {{.*}}source-a-opt.o:(.init_array.200)
