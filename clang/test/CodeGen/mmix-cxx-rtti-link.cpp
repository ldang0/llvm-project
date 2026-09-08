// REQUIRES: mmix-registered-target
// RUN: split-file %s %t
// RUN: %clang_cc1 -triple mmix -mrelocation-model static -std=c++17 -O0 -emit-obj -o %t/first-O0.o %t/first.cpp
// RUN: %clang_cc1 -triple mmix -mrelocation-model static -std=c++17 -O0 -emit-obj -o %t/second-O0.o %t/second.cpp
// RUN: %clang_cc1 -triple mmix -mrelocation-model static -std=c++17 -O0 -emit-obj -o %t/entry-O0.o %t/entry.cpp
// RUN: %clang_cc1 -triple mmix -mrelocation-model static -std=c++17 -O0 -emit-obj -o %t/runtime-O0.o %t/runtime.cpp
// RUN: llvm-readobj --section-groups --symbols --relocations %t/first-O0.o | FileCheck %s --check-prefixes=OBJECT,CONSTRUCTION
// RUN: ld.lld -m elf64mmix -e entry -o %t/direct-O0 %t/entry-O0.o %t/first-O0.o %t/second-O0.o %t/runtime-O0.o
// RUN: llvm-nm --undefined-only %t/direct-O0 | count 0
// RUN: llvm-nm --defined-only %t/direct-O0 | FileCheck %s --check-prefix=LINK
// RUN: llvm-ar cr %t/owners-O0.a %t/first-O0.o %t/second-O0.o
// RUN: ld.lld -m elf64mmix -e entry -o %t/archive-O0 %t/entry-O0.o %t/owners-O0.a %t/runtime-O0.o
// RUN: llvm-nm --defined-only %t/archive-O0 | FileCheck %s --check-prefix=LINK
// RUN: llvm-nm --undefined-only %t/archive-O0 | count 0
// RUN: ld.lld -m elf64mmix -r -o %t/partial-O0.o %t/first-O0.o %t/second-O0.o
// RUN: ld.lld -m elf64mmix -e entry -o %t/final-O0 %t/entry-O0.o %t/partial-O0.o %t/runtime-O0.o
// RUN: llvm-nm --defined-only %t/final-O0 | FileCheck %s --check-prefix=LINK
// RUN: llvm-nm --undefined-only %t/final-O0 | count 0
// RUN: %clang_cc1 -triple mmix -mrelocation-model static -std=c++17 -O0 -emit-llvm-bc -o %t/first-O0.bc %t/first.cpp
// RUN: %clang_cc1 -triple mmix -mrelocation-model static -std=c++17 -O0 -emit-llvm-bc -o %t/second-O0.bc %t/second.cpp
// RUN: %clang_cc1 -triple mmix -mrelocation-model static -std=c++17 -O0 -emit-llvm-bc -o %t/entry-O0.bc %t/entry.cpp
// RUN: ld.lld -m elf64mmix -e entry --lto-O0 -o %t/lto-O0 %t/entry-O0.bc %t/first-O0.bc %t/second-O0.bc %t/runtime-O0.o
// RUN: llvm-nm --undefined-only %t/lto-O0 | count 0
// RUN: %clang_cc1 -triple mmix -mrelocation-model static -std=c++17 -O2 -emit-obj -o %t/first-O2.o %t/first.cpp
// RUN: %clang_cc1 -triple mmix -mrelocation-model static -std=c++17 -O2 -emit-obj -o %t/second-O2.o %t/second.cpp
// RUN: %clang_cc1 -triple mmix -mrelocation-model static -std=c++17 -O2 -emit-obj -o %t/entry-O2.o %t/entry.cpp
// RUN: %clang_cc1 -triple mmix -mrelocation-model static -std=c++17 -O2 -emit-obj -o %t/runtime-O2.o %t/runtime.cpp
// RUN: llvm-readobj --section-groups --symbols --relocations %t/first-O2.o | FileCheck %s --check-prefix=OBJECT
// RUN: ld.lld -m elf64mmix -e entry -o %t/direct-O2 %t/entry-O2.o %t/first-O2.o %t/second-O2.o %t/runtime-O2.o
// RUN: llvm-nm --undefined-only %t/direct-O2 | count 0
// RUN: llvm-nm --defined-only %t/direct-O2 | FileCheck %s --check-prefix=LINK
// RUN: llvm-ar cr %t/owners-O2.a %t/first-O2.o %t/second-O2.o
// RUN: ld.lld -m elf64mmix -e entry -o %t/archive-O2 %t/entry-O2.o %t/owners-O2.a %t/runtime-O2.o
// RUN: llvm-nm --defined-only %t/archive-O2 | FileCheck %s --check-prefix=LINK
// RUN: llvm-nm --undefined-only %t/archive-O2 | count 0
// RUN: ld.lld -m elf64mmix -r -o %t/partial-O2.o %t/first-O2.o %t/second-O2.o
// RUN: ld.lld -m elf64mmix -e entry -o %t/final-O2 %t/entry-O2.o %t/partial-O2.o %t/runtime-O2.o
// RUN: llvm-nm --defined-only %t/final-O2 | FileCheck %s --check-prefix=LINK
// RUN: llvm-nm --undefined-only %t/final-O2 | count 0
// RUN: %clang_cc1 -triple mmix -mrelocation-model static -std=c++17 -O2 -emit-llvm-bc -o %t/first-O2.bc %t/first.cpp
// RUN: %clang_cc1 -triple mmix -mrelocation-model static -std=c++17 -O2 -emit-llvm-bc -o %t/second-O2.bc %t/second.cpp
// RUN: %clang_cc1 -triple mmix -mrelocation-model static -std=c++17 -O2 -emit-llvm-bc -o %t/entry-O2.bc %t/entry.cpp
// RUN: ld.lld -m elf64mmix -e entry --lto-O2 -o %t/lto-O2 %t/entry-O2.bc %t/first-O2.bc %t/second-O2.bc %t/runtime-O2.o
// RUN: llvm-nm --undefined-only %t/lto-O2 | count 0

// OBJECT-DAG: Signature: _ZTI4Poly
// OBJECT-DAG: Name: _ZTI4Poly
// OBJECT-DAG: Binding: Weak
// OBJECT-DAG: R_MMIX_64 _ZTVN10__cxxabiv117__class_type_infoE 0x10
// OBJECT-DAG: Signature: _ZTS4Poly
// OBJECT-DAG: Signature: _ZTV4Poly
// CONSTRUCTION-DAG: Signature: _ZTT4Poly
// CONSTRUCTION-DAG: Name: _ZTC4Poly0_6Middle
// OBJECT-DAG: STV_HIDDEN
// LINK-COUNT-1: r _ZTI4Poly
// LINK-NOT: _ZTI4Poly

//--- shared.h
namespace std { class type_info; }
inline void *operator new(__SIZE_TYPE__, void *p) { return p; }
struct Root { virtual long value() { return 1; } };
struct Middle : virtual Root { long value() override { return 3; } };
struct __attribute__((visibility("hidden"))) Poly : Middle {
  long value() override { return 7; }
};
extern const std::type_info *first();
extern const std::type_info *second();

//--- first.cpp
#include "shared.h"
const std::type_info *first() { return &typeid(Poly); }
void construct_first(void *p) { new (p) Poly; }

//--- second.cpp
#include "shared.h"
const std::type_info *second() { return &typeid(Poly); }
void construct_second(void *p) { new (p) Poly; }

//--- entry.cpp
#include "shared.h"
extern "C" int entry() { return first() != second(); }

//--- runtime.cpp
// Link inspection only: this storage does not implement a type-info vtable.
extern "C" {
void *class_vtable[3] asm("_ZTVN10__cxxabiv117__class_type_infoE") = {};
void *single_vtable[3] asm("_ZTVN10__cxxabiv120__si_class_type_infoE") = {};
void *multiple_vtable[3] asm("_ZTVN10__cxxabiv121__vmi_class_type_infoE") = {};
}
