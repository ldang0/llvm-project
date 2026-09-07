// REQUIRES: mmix-registered-target
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_LOCAL_INHERITANCE %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=INHERITANCE
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_GLOBAL_INHERITANCE %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=INHERITANCE
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_VIRTUAL %s 2>&1 | FileCheck %s --check-prefix=VIRTUAL

struct Left {
  long value;
};

struct Base {
  long method(long);
};

struct Derived : Left, Base {};

using BaseMember = long (Base::*)(long);
using DerivedMember = long (Derived::*)(long);

#if defined(TEST_LOCAL_INHERITANCE)
DerivedMember convert(BaseMember member) { return member; }
#elif defined(TEST_GLOBAL_INHERITANCE)
DerivedMember member = &Base::method;
#elif defined(TEST_VIRTUAL)
struct Polymorphic {
  virtual long method(long);
};
using VirtualMember = long (Polymorphic::*)(long);
VirtualMember member = &Polymorphic::method;
#endif

// INHERITANCE: error: MMIX C++ producer profile does not support member-function-pointer inheritance conversions
// VIRTUAL: error: MMIX C++ producer profile does not support virtual member-function pointers
