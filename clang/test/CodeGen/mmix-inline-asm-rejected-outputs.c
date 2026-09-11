// RUN: not %clang_cc1 -triple mmix -mrelocation-model static -ffreestanding -O0 -S -o /dev/null \
// RUN:   -mllvm -verify-machineinstrs %s 2>&1 | FileCheck %s --implicit-check-not="Stack dump" --implicit-check-not="Bad machine code"
// RUN: not %clang_cc1 -triple mmix -mrelocation-model static -ffreestanding -O2 -S -o /dev/null \
// RUN:   -mllvm -verify-machineinstrs %s 2>&1 | FileCheck %s --implicit-check-not="Stack dump" --implicit-check-not="Bad machine code"

// A rejected transition with tied outputs must not leave the returned value
// reading a virtual register without a definition.
// CHECK: fatal error: error in backend: MMIX instruction 'TRAP' is only permitted in module-level inline assembly
long rejected_transition(long value, long number) {
  register long result __asm__("$231") = value;
  register long service __asm__("$237") = number;
  __asm__ volatile("TRAP 1,0,0" : "+r"(result), "+r"(service) : : "memory");
  return result;
}
