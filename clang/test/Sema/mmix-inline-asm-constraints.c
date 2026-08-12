// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple mmix-unknown-unknown -DCODEGEN \
// RUN:   -mrelocation-model static -emit-llvm %s -o - | FileCheck %s
// RUN: %clang_cc1 -triple mmix-unknown-unknown -DCODEGEN \
// RUN:   -mrelocation-model static -emit-obj %s -o /dev/null
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -DINVALID_FLOAT \
// RUN:   -mrelocation-model static -emit-obj %s -o /dev/null 2>&1 | \
// RUN:   FileCheck %s --check-prefix=INVALID-FLOAT

#if defined(INVALID_FLOAT)

void invalid_float_zero(void) {
  __asm__ volatile("SWYM 0, 0, 0" : : "G"(1.0));
}

// INVALID-FLOAT: error: invalid operand for inline asm constraint 'G'

#else

long constraint_value;
long constraint_object;

void accepted_constraints(void) {
  long value = constraint_value;
  long *address = &constraint_object;
  long output;
  __asm__ volatile("" : "=&r"(output) : "r"(value));
  __asm__ volatile("" : "+r"(output));
  __asm__ volatile("" : "=r"(output) : "0"(value));
  __asm__ volatile("" : "=r,r"(output) : "r,I"(value));
  __asm__ volatile("" : "=m"(*address));
  __asm__ volatile("" : "=o"(*address));
  __asm__ volatile("" : : "m"(*address), "o"(*address), "Vr"(*address),
                   "p"(address));
  __asm__ volatile("" : : "I"(0), "I"(255), "J"(0), "J"(65535),
                   "K"(-255), "K"(0), "M"(0), "O"(3), "O"(5), "O"(9),
                   "O"(17), "G"(0.0));
}

// CHECK-LABEL: define{{.*}} void @accepted_constraints
// CHECK: call i64 asm sideeffect "", "=&r,r"
// CHECK-COUNT-2: call i64 asm sideeffect "", "=r,0"
// CHECK: call i64 asm sideeffect "", "=r|r,r|I"
// CHECK: call void asm sideeffect "", "=*m"
// CHECK: call void asm sideeffect "", "=*o"
// CHECK: call void asm sideeffect "", "*m,*o,Vr,p"
// CHECK: call void asm sideeffect "", "I,I,J,J,K,K,M,O,O,O,O,G"

#ifndef CODEGEN

void invalid_immediates(void) {
  __asm__ volatile("" : : "I"(-1)); // expected-error {{value '-1' out of range for constraint 'I'}}
  __asm__ volatile("" : : "I"(256)); // expected-error {{value '256' out of range for constraint 'I'}}
  __asm__ volatile("" : : "J"(-1)); // expected-error {{value '-1' out of range for constraint 'J'}}
  __asm__ volatile("" : : "J"(65536)); // expected-error {{value '65536' out of range for constraint 'J'}}
  __asm__ volatile("" : : "K"(-256)); // expected-error {{value '-256' out of range for constraint 'K'}}
  __asm__ volatile("" : : "K"(1)); // expected-error {{value '1' out of range for constraint 'K'}}
  __asm__ volatile("" : : "M"(1)); // expected-error {{value '1' out of range for constraint 'M'}}
  __asm__ volatile("" : : "O"(4)); // expected-error {{value '4' out of range for constraint 'O'}}
}

void invalid_memory(long value) {
  __asm__ volatile("" : : "m"(value + 1)); // expected-error {{invalid lvalue in asm input for constraint 'm'}}
  __asm__ volatile("" : "=o"(value + 1)); // expected-error {{invalid lvalue in asm output}}
}

void internal_constraints(long value) {
  __asm__ volatile("" : : "x"(value)); // expected-error {{invalid input constraint 'x' in asm}}
  __asm__ volatile("" : : "y"(value)); // expected-error {{invalid input constraint 'y' in asm}}
  __asm__ volatile("" : : "z"(value)); // expected-error {{invalid input constraint 'z' in asm}}
  __asm__ volatile("" : : "L"(value)); // expected-error {{invalid input constraint 'L' in asm}}
  __asm__ volatile("" : : "N"(value)); // expected-error {{invalid input constraint 'N' in asm}}
  __asm__ volatile("" : : "R"(value)); // expected-error {{invalid input constraint 'R' in asm}}
  __asm__ volatile("" : : "S"(value)); // expected-error {{invalid input constraint 'S' in asm}}
  __asm__ volatile("" : : "T"(value)); // expected-error {{invalid input constraint 'T' in asm}}
  __asm__ volatile("" : : "U"(value)); // expected-error {{invalid input constraint 'U' in asm}}
  __asm__ volatile("" : : "Yf"(value)); // expected-error {{invalid input constraint 'Yf' in asm}}
}

#endif
#endif
