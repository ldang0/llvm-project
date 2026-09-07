// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c++17 \
// RUN:   -fsyntax-only -verify %s

struct Base {
  long value;
};

struct Derived : virtual Base {};

long Derived::*to_derived(long Base::*member) {
  return member;
  // expected-error@-1 {{conversion from pointer to member of class 'Base' to pointer to member of class 'Derived' via virtual base 'Base' is not allowed}}
}
