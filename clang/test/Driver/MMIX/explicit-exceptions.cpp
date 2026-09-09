// REQUIRES: mmix-registered-target
// RUN: %clangxx --target=mmix -ffreestanding -fexceptions -c %s -o %t.o
// RUN: %clangxx --target=mmix -ffreestanding -fcxx-exceptions -c %s -o %t.cxx.o
// RUN: %clangxx --target=mmix -ffreestanding -fexceptions -### -c %s 2>&1 | FileCheck %s --check-prefix=MODEL
// RUN: not %clangxx --target=mmix -ffreestanding -fexceptions -fno-exceptions -c %s -o %t.no.o 2>&1 | FileCheck %s --check-prefix=DISABLED
// RUN: not %clangxx --target=mmix --sysroot=%t.sysroot -fexceptions %s -o %t 2>&1 | FileCheck %s --check-prefix=LINK
// RUN: %clangxx --target=mmix -fexceptions -ffreestanding -nostdlib -nostartfiles -nodefaultlibs -### %s 2>&1 | FileCheck %s --check-prefix=MANUAL
// MODEL: "-exception-model=dwarf"
// DISABLED: error: cannot use 'throw' with exceptions disabled
// LINK: automatic MMIX exception runtime selection; use explicit runtime inputs and -nostdlib++
// MANUAL-NOT: libc++abi.a
// MANUAL-NOT: crtdso
// MANUAL: ld.lld
// MANUAL-NOT: libc++abi.a
// MANUAL-NOT: crtdso
void raise() { throw 42; }
