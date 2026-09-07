// REQUIRES: mmix-registered-target
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_DYNAMIC_INITIALIZATION %s 2>&1 | FileCheck %s --check-prefix=INIT
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_POLYMORPHIC_BOUNDARY %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=BOUNDARY

#if defined(TEST_DYNAMIC_INITIALIZATION)
extern long make_value();
long value = make_value();
// INIT: error: MMIX C++ producer profile does not support dynamic initialization
#elif defined(TEST_POLYMORPHIC_BOUNDARY)
struct Base {
  virtual long value();
};
Base pass(Base object) { return object; }
// BOUNDARY: error: MMIX C++ producer profile does not support polymorphic record call boundaries
#endif
