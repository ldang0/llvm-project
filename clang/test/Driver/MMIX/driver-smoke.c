// RUN: %clang -### --target=mmix-unknown-unknown -ffreestanding -c \
// RUN:   %S/Inputs/freestanding.c -o %t.o 2>&1 | FileCheck %s
// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -c \
// RUN:   %S/Inputs/freestanding.c -o %t.o

// CHECK: (in-process)
// CHECK-NEXT: {{.*}}clang{{.*}} "-cc1" "-triple" "mmix-unknown-unknown" "-emit-obj"
// CHECK-SAME: "-mrelocation-model" "static"
// CHECK-SAME: "-o" "{{.*}}.o"
// CHECK-SAME: "-x" "c" "{{.*}}Inputs{{/|\\}}freestanding.c"
