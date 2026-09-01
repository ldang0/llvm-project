// REQUIRES: mmix-registered-target

// RUN: rm -rf %t && split-file %s %t
// RUN: %clangxx --target=mmix-unknown-unknown -std=c++17 -O2 -flto \
// RUN:   -ffreestanding -fno-exceptions -fno-rtti -c %t/a.cpp -o %t/a.bc
// RUN: %clangxx --target=mmix-unknown-unknown -std=c++17 -O2 -flto \
// RUN:   -ffreestanding -fno-exceptions -fno-rtti -c %t/b.cpp -o %t/b.bc
// RUN: env PATH=/usr/bin:/bin %clangxx --target=mmix-unknown-unknown -O2 -flto \
// RUN:   -ffreestanding -fno-exceptions -fno-rtti -nostdlib -nostartfiles \
// RUN:   -nodefaultlibs %t/a.bc %t/b.bc -Wl,-e,from_a \
// RUN:   -Wl,-u,from_b,-u,_Z7add_oneIlET_S0_ -o %t/output
// RUN: llvm-readobj --file-headers --symbols --relocations %t/output \
// RUN:   | FileCheck %s --implicit-check-not='{{(libstdc\+\+|libc\+\+|__cxa_)}}'

// CHECK:      Format: elf64-mmix
// CHECK:      Type: Executable
// CHECK:      Relocations [
// CHECK-NEXT: ]
// CHECK-DAG:  Name: from_b
// CHECK-COUNT-1: Name: _Z7add_oneIlET_S0_
// CHECK-DAG:  Name: from_a

//--- template.h
template <class T>
__attribute__((noinline, used, visibility("default"))) T add_one(T value) {
  return value + 1;
}

//--- a.cpp
#include "template.h"
extern "C" long from_a(long value) { return add_one(value); }

//--- b.cpp
#include "template.h"
extern "C" long from_b(long value) { return add_one(value); }
