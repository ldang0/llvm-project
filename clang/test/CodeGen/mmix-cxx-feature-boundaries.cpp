// REQUIRES: mmix-registered-target
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_DYNAMIC_INITIALIZATION %s 2>&1 | FileCheck %s --check-prefix=INIT
extern long make_value();
long value = make_value();
// INIT: error: MMIX C++ producer profile does not support dynamic initialization
