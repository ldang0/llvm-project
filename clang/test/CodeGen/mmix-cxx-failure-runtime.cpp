// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 -fno-rtti \
// RUN:   -mrelocation-model static -emit-obj -O0 -o %t.o %s
// RUN: llvm-nm --undefined-only %t.o | FileCheck %s

// Generic libc++abi owns virtual-call failure and allocation behavior.
struct Abstract {
  virtual ~Abstract();
  virtual long value() const = 0;
};

Abstract::~Abstract() {}

// CHECK: U __cxa_pure_virtual
