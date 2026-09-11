// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -std=c17 \
// RUN:   -E -dM %s -o - | FileCheck %s --check-prefix=MACROS
// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -std=c17 -O0 \
// RUN:   -S -emit-llvm %s -o - | FileCheck %s --check-prefix=IR
// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -std=c17 -O0 \
// RUN:   -c %s -o %t.o
// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -std=c17 -O2 \
// RUN:   -c %s -o %t.o
// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -std=c17 \
// RUN:   -DTEST_OVERALIGNED -O0 -c %s -o %t.overaligned.o
// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -std=c17 \
// RUN:   -DTEST_OVERALIGNED -O2 -c %s -o %t.overaligned.o
// RUN: not %clang --target=mmix-unknown-unknown -ffreestanding -std=c17 \
// RUN:   -fstack-clash-protection -c %s -o %t.stack-clash.o 2>&1 \
// RUN:   | FileCheck %s --check-prefix=STACK-CLASH
// RUN: not test -e %t.stack-clash.o
// RUN: not %clang --target=mmix-unknown-unknown -ffreestanding -std=c17 \
// RUN:   -fsplit-stack -c %s -o %t.split-stack.o 2>&1 \
// RUN:   | FileCheck %s --check-prefix=SPLIT-STACK
// RUN: not test -e %t.split-stack.o
// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -std=c17 \
// RUN:   -fno-stack-clash-protection -fno-split-stack -c %s \
// RUN:   -o %t.disabled-hardening.o

// MACROS-NOT: #define __STDC_NO_VLA__

// STACK-CLASH: error: unsupported option '-fstack-clash-protection' for target 'mmix-unknown-unknown'
// SPLIT-STACK: error: unsupported option '-fsplit-stack' for target 'mmix-unknown-unknown'
// MACROS: #define __STDC_VERSION__ 201710L
// MACROS-NOT: #define __STDC_NO_VLA__

// IR-LABEL: define{{.*}} i64 @vla_surface(
// IR: call ptr @llvm.stacksave.p0()
// IR: alloca i32, i64 %{{.*}}, align 4
// IR: mul nuw i64
// IR: call void @llvm.stackrestore.p0(
long vla_surface(int rows, int columns, int parameter[][columns]) {
  int values[rows][columns];
  int (*pointer)[columns] = values;
  long bytes = sizeof(values);
  pointer[rows - 1][columns - 1] = parameter[0][0];
  return pointer[rows - 1][columns - 1] + bytes;
}

// IR-LABEL: define{{.*}} i32 @nested_vla(
// IR: call ptr @llvm.stacksave.p0()
// IR: alloca i8, i64 %{{.*}}, align 1
// IR: call ptr @llvm.stacksave.p0()
// IR: alloca i64, i64 %{{.*}}, align 8
// IR: call void @llvm.stackrestore.p0(
// IR: call void @llvm.stackrestore.p0(
int nested_vla(int count) {
  int result;
  {
    unsigned char outer[count];
    outer[0] = 3;
    {
      unsigned long inner[count];
      inner[0] = 5;
      result = outer[0] + inner[0];
    }
  }
  return result;
}

#if defined(TEST_OVERALIGNED)
int overaligned_vla(int count) {
  _Alignas(16) volatile unsigned char values[count];
  values[0] = 1;
  return values[0];
}
#endif
