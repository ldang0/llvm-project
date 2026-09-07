// REQUIRES: mmix-registered-target
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null %s 2>&1 \
// RUN:   | FileCheck %s

struct Value {
  int field;
  virtual int get() const;
};

int read(Value value) { return value.get(); }

// CHECK: error: MMIX C++ producer profile does not support polymorphic record call boundaries
