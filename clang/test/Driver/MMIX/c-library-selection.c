// RUN: %clang -### --target=mmix-unknown-unknown -ffreestanding \
// RUN:   --cstdlib=newlib -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=NEWLIB \
// RUN:       --implicit-check-not='argument unused' \
// RUN:       --implicit-check-not=error:

// RUN: not %clang -### --target=mmix-unknown-unknown -ffreestanding \
// RUN:   --cstdlib=picolibc -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=PICOLIBC \
// RUN:       --implicit-check-not='argument unused'
// RUN: not %clang -### --target=mmix-unknown-unknown -ffreestanding \
// RUN:   --cstdlib=llvm-libc -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=LLVM-LIBC \
// RUN:       --implicit-check-not='argument unused'
// RUN: not %clang -### --target=mmix-unknown-unknown -ffreestanding \
// RUN:   --cstdlib=system -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=SYSTEM \
// RUN:       --implicit-check-not='argument unused'

// RUN: not %clang -### --target=mmix-unknown-unknown -ffreestanding \
// RUN:   --cstdlib=invalid -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=INVALID \
// RUN:       --implicit-check-not='unsupported option' \
// RUN:       --implicit-check-not='argument unused'

// NEWLIB: "-cc1" "-triple" "mmix-unknown-unknown"
// PICOLIBC: error: unsupported option '--cstdlib=picolibc' for target 'mmix-unknown-unknown'
// LLVM-LIBC: error: unsupported option '--cstdlib=llvm-libc' for target 'mmix-unknown-unknown'
// SYSTEM: error: unsupported option '--cstdlib=system' for target 'mmix-unknown-unknown'
// INVALID: error: invalid C library name in argument '--cstdlib=invalid'

int value;
