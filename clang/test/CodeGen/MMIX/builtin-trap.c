// RUN: %clang_cc1 -triple mmix -mrelocation-model static -emit-llvm -O0 %s -o - | FileCheck %s --check-prefix=IR
// RUN: %clang_cc1 -triple mmix -mrelocation-model static -emit-llvm -O2 %s -o - | FileCheck %s --check-prefix=IR
// RUN: %clang -target mmix -fno-pic -S -O0 %s -o - | FileCheck %s --check-prefix=ASM
// RUN: %clang -target mmix -fno-pic -S -O2 %s -o - | FileCheck %s --check-prefix=ASM

void trap(void) {
  // IR: call void @llvm.trap()
  // IR-NEXT: unreachable
  // ASM-LABEL: trap:
  // ASM: TRAP 255, 0, 0
  // ASM-NOT: POP
  __builtin_trap();
}
