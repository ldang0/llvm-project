// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c11 -fsyntax-only \
// RUN:   -verify %s

_Thread_local int tls; // expected-error {{thread-local storage is not supported for the current target}}

void variable_length_array(int n) {
  int values[n]; // expected-error {{variable length arrays are not supported for the current target}}
  (void)values;
}

__attribute__((target_clones("default", "base")))
void target_clones_attribute(void) {} // expected-error {{function multiversioning is not supported on the current target}}

__attribute__((cpu_specific(generic))) // expected-error {{invalid option 'generic' for cpu_specific}}
void cpu_specific_attribute(void) {}

__attribute__((interrupt)) // expected-warning {{unknown attribute 'interrupt' ignored}}
void interrupt_attribute(void) {}

__attribute__((fastcall)) // expected-error {{'fastcall' calling convention is not supported for this target}}
void alternate_calling_convention(void) {}

__attribute__((cdecl)) void c_calling_convention(void) {}

void mandatory_tail_call(void) {
  [[clang::musttail]] return mandatory_tail_call(); // expected-warning {{unknown attribute 'clang::musttail' ignored}}
}
