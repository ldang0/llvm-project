// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple mmix-unknown-unknown -DCODEGEN \
// RUN:   -mrelocation-model static \
// RUN:   -emit-llvm %s -o - | FileCheck %s

void accepted_register_names(void) {
  __asm__ volatile("" ::: "$0", "$1", "$253", "$254", "$255", "sp", "rD",
                   "rE", "rH", "rJ", "rR", "rO");
}

// CHECK-LABEL: define{{.*}} void @accepted_register_names()
// CHECK: call void asm sideeffect "", "~{r0},~{r1},~{r253},~{r254},~{r255},~{r254},~{rD},~{rE},~{rH},~{rJ},~{rR},~{rO}"()

#ifndef CODEGEN
void rejected_register_names(void) {
  __asm__ volatile("" ::: "$256"); // expected-error {{unknown register name '$256' in asm}}
  __asm__ volatile("" ::: "r0"); // expected-error {{unknown register name 'r0' in asm}}
  __asm__ volatile("" ::: "r255"); // expected-error {{unknown register name 'r255' in asm}}
  __asm__ volatile("" ::: ":sp"); // expected-error {{unknown register name ':sp' in asm}}
  __asm__ volatile("" ::: "rA"); // expected-error {{unknown register name 'rA' in asm}}
  __asm__ volatile("" ::: "rG"); // expected-error {{unknown register name 'rG' in asm}}
}
#endif
