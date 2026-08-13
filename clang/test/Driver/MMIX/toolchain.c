// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -c \
// RUN:   %S/Inputs/freestanding.c -o %t-input.o
// RUN: not %clang -### --target=mmix-unknown-unknown %t-input.o \
// RUN:   -o %t 2>&1 | FileCheck --check-prefix=LINK %s \
// RUN:     --implicit-check-not='{{[/\\](gcc|ld|as)[^/\\"]*"}}'
// RUN: not %clang -### --target=mmix-unknown-unknown -static \
// RUN:   %S/Inputs/freestanding.c -o %t 2>&1 \
// RUN:   | FileCheck --check-prefix=LINK %s \
// RUN:       --implicit-check-not='{{[/\\](gcc|ld|as)[^/\\"]*"}}'
// RUN: not %clang -### --target=mmix-unknown-unknown -r \
// RUN:   %S/Inputs/freestanding.c -o %t.o 2>&1 \
// RUN:   | FileCheck --check-prefix=LINK %s \
// RUN:       --implicit-check-not='{{[/\\](gcc|ld|as)[^/\\"]*"}}'
// RUN: not %clang -### --target=mmix-unknown-unknown -shared \
// RUN:   %S/Inputs/freestanding.c -o %t.so 2>&1 \
// RUN:   | FileCheck --check-prefix=LINK %s \
// RUN:       --implicit-check-not='{{[/\\](gcc|ld|as)[^/\\"]*"}}'
// RUN: not %clang -### --target=mmix-unknown-unknown -pie \
// RUN:   %S/Inputs/freestanding.c -o %t 2>&1 \
// RUN:   | FileCheck --check-prefix=LINK %s \
// RUN:       --implicit-check-not='{{[/\\](gcc|ld|as)[^/\\"]*"}}'
// RUN: not %clang -### --target=mmix-unknown-unknown \
// RUN:   --sysroot=%t-sysroot -nostdlib -nostartfiles -nodefaultlibs \
// RUN:   -rtlib=compiler-rt -fuse-ld=lld %S/Inputs/freestanding.c \
// RUN:   -o %t 2>&1 | FileCheck --check-prefix=LINK %s \
// RUN:     --implicit-check-not='{{[/\\](gcc|ld|as)[^/\\"]*"}}'
// RUN: not %clang -### --target=mmix-unknown-unknown -fno-integrated-as -c \
// RUN:   %S/Inputs/freestanding.c -o %t.o 2>&1 \
// RUN:   | FileCheck --check-prefix=AS %s \
// RUN:       --implicit-check-not='{{[/\\](gcc|ld|as)[^/\\"]*"}}'
// RUN: not %clang -### --target=mmix-unknown-unknown -flto \
// RUN:   %S/Inputs/freestanding.c -o %t-lto 2>&1 \
// RUN:   | FileCheck --check-prefix=LINK %s \
// RUN:       --implicit-check-not='{{[/\\](gcc|ld|as)[^/\\"]*"}}'
// RUN: not %clang -### --target=mmix-unknown-unknown -flto=thin \
// RUN:   %S/Inputs/freestanding.c -o %t-thinlto 2>&1 \
// RUN:   | FileCheck --check-prefix=LINK %s \
// RUN:       --implicit-check-not='{{[/\\](gcc|ld|as)[^/\\"]*"}}'

// LINK: error: the clang compiler does not support 'linking for MMIX'

// AS: error: the clang compiler does not support 'external assembly for MMIX'
