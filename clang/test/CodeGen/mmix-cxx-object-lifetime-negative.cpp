// REQUIRES: mmix-registered-target
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_GENERAL_ALLOCATION %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=GENERAL-ALLOCATION
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_VIRTUAL_INHERITANCE %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=VIRTUAL-INHERITANCE

#if defined(TEST_GENERAL_ALLOCATION)
using size_t = decltype(sizeof(0));
void *operator new(size_t);

long *allocate() { return new long; }
// GENERAL-ALLOCATION: error: MMIX C++ producer profile does not support general allocation
#elif defined(TEST_VIRTUAL_INHERITANCE)
struct Base {};
struct Derived : virtual Base {};

void construct() { Derived Value; }
// VIRTUAL-INHERITANCE: error: MMIX C++ producer profile does not support virtual-base construction and destruction
#endif
