// RUN: not %clang -### --target=mmix-unknown-unknown \
// RUN:   %S/Inputs/freestanding.c -o %t 2>&1 | FileCheck --check-prefix=LINK %s
// RUN: not %clang -### --target=mmix-unknown-unknown -r \
// RUN:   %S/Inputs/freestanding.c -o %t.o 2>&1 | FileCheck --check-prefix=LINK %s
// RUN: not %clang -### --target=mmix-unknown-unknown -shared \
// RUN:   %S/Inputs/freestanding.c -o %t.so 2>&1 | FileCheck --check-prefix=LINK %s
// RUN: not %clang -### --target=mmix-unknown-unknown -pie \
// RUN:   %S/Inputs/freestanding.c -o %t 2>&1 | FileCheck --check-prefix=LINK %s
// RUN: not %clang -### --target=mmix-unknown-unknown -fno-integrated-as -c \
// RUN:   %S/Inputs/freestanding.c -o %t.o 2>&1 | FileCheck --check-prefix=AS %s

// LINK: error: the clang compiler does not support 'linking for MMIX'
// LINK-NOT: {{[/\\]gcc[^/\\"]*"}}
// LINK-NOT: {{[/\\]ld[^/\\"]*"}}

// AS: error: the clang compiler does not support 'external assembly for MMIX'
// AS-NOT: {{[/\\]as[^/\\"]*"}}
