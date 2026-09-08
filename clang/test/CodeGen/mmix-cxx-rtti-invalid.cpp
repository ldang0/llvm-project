// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix -std=c++17 -fsyntax-only -verify %s

struct Base { virtual void f(); };
struct Left : Base {};
struct Right : Base {};
struct Ambiguous : Left, Right {};
struct Private : private Base {}; // expected-note {{declared private here}}
struct Plain {};
struct Incomplete; // expected-note {{forward declaration}}

Base *ambiguous(Ambiguous *p) {
  return dynamic_cast<Base *>(p); // expected-error {{ambiguous conversion}}
}
Base *inaccessible(Private *p) {
  return dynamic_cast<Base *>(p); // expected-error {{cannot cast 'Private' to its private base class 'Base'}}
}
Base *nonpolymorphic(Plain *p) {
  return dynamic_cast<Base *>(p); // expected-error {{'Plain' is not polymorphic}}
}
Incomplete *incomplete(Base *p) {
  return dynamic_cast<Incomplete *>(p); // expected-error {{'Incomplete' is an incomplete type}}
}
