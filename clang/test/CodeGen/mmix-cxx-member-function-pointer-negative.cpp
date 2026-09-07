// REQUIRES: mmix-registered-target
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -mrelocation-model static -emit-llvm -o /dev/null \
// RUN:   -DTEST_VIRTUAL %s 2>&1 | FileCheck %s --check-prefix=VIRTUAL
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -DTEST_VIRTUAL_BASE -fsyntax-only -verify %s

#if defined(TEST_VIRTUAL)
struct Polymorphic {
  virtual long method(long);
};
using VirtualMember = long (Polymorphic::*)(long);
VirtualMember member = &Polymorphic::method;
#elif defined(TEST_VIRTUAL_BASE)
struct Base {
  long method(long);
};
struct Derived : virtual Base {};
using BaseMember = long (Base::*)(long);
using DerivedMember = long (Derived::*)(long);
DerivedMember convert(BaseMember member) {
  return member;
  // expected-error@-1 {{conversion from pointer to member of class 'Base' to pointer to member of class 'Derived' via virtual base 'Base' is not allowed}}
}
#endif

// VIRTUAL: error: MMIX C++ producer profile does not support virtual member-function pointers
