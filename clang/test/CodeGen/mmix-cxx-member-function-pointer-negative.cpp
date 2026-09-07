// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -DTEST_VIRTUAL_BASE -fsyntax-only -verify %s

#if defined(TEST_VIRTUAL_BASE)
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
