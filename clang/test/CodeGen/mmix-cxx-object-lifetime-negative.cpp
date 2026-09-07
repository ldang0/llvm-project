// REQUIRES: mmix-registered-target
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_GENERAL_ALLOCATION %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=GENERAL-ALLOCATION
using size_t = decltype(sizeof(0));
void *operator new(size_t);

long *allocate() { return new long; }
// GENERAL-ALLOCATION: error: MMIX C++ producer profile does not support general allocation
