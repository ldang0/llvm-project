// RUN: %clang --target=mmix-unknown-unknown -ffreestanding \
// RUN:   -fno-stack-protector -O0 -c %s -o %t.o
// RUN: llvm-readobj --file-headers %t.o | FileCheck %s

// CHECK: Format: elf64-mmix
// CHECK: Arch: mmix
// CHECK: Type: Relocatable

_Complex float multiply(_Complex float lhs, _Complex float rhs) {
  return lhs * rhs;
}

extern float combine(float, float);
extern float real_part(_Complex float);
extern float imaginary_part(_Complex float);

float magnitude(_Complex float value) {
  return combine(real_part(value), imaginary_part(value));
}
