// RUN: %clang -### --target=mmix-unknown-unknown -ffreestanding \
// RUN:   -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ACCEPTED \
// RUN:       --implicit-check-not='argument unused' \
// RUN:       --implicit-check-not=error: \
// RUN:       --implicit-check-not=ld.lld
// RUN: %clang -### --target=mmix-unknown-unknown -ffreestanding \
// RUN:   --cstdlib=newlib -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ACCEPTED \
// RUN:       --implicit-check-not='argument unused' \
// RUN:       --implicit-check-not=error: \
// RUN:       --implicit-check-not=ld.lld
// RUN: %clang -### --target=mmix-unknown-unknown -ffreestanding \
// RUN:   --cstdlib=llvm-libc -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ACCEPTED \
// RUN:       --implicit-check-not='argument unused' \
// RUN:       --implicit-check-not=error: \
// RUN:       --implicit-check-not=ld.lld
// RUN: %clang -### --target=mmix-unknown-elf -ffreestanding \
// RUN:   --cstdlib=llvm-libc -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ACCEPTED \
// RUN:       --implicit-check-not='argument unused' \
// RUN:       --implicit-check-not=error: \
// RUN:       --implicit-check-not=ld.lld

// RUN: %clang -### --target=mmix-unknown-unknown -ffreestanding \
// RUN:   --cstdlib=picolibc --cstdlib=llvm-libc -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ACCEPTED \
// RUN:       --implicit-check-not='argument unused' \
// RUN:       --implicit-check-not=error: \
// RUN:       --implicit-check-not=ld.lld
// RUN: %clang -### --target=mmix-unknown-unknown -ffreestanding \
// RUN:   --cstdlib=llvm-libc --cstdlib=newlib -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ACCEPTED \
// RUN:       --implicit-check-not='argument unused' \
// RUN:       --implicit-check-not=error: \
// RUN:       --implicit-check-not=ld.lld

// RUN: not %clang -### --target=mmix-unknown-unknown -ffreestanding \
// RUN:   --cstdlib=picolibc -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=PICOLIBC \
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
// RUN: not %clang -### --target=mmix-unknown-unknown -ffreestanding \
// RUN:   --cstdlib=llvm-libc --cstdlib=system -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=SYSTEM \
// RUN:       --implicit-check-not='argument unused'

// ACCEPTED: "-cc1" "-triple" "mmix-unknown-unknown"
// PICOLIBC: error: unsupported option '--cstdlib=picolibc' for target 'mmix-unknown-unknown'
// SYSTEM: error: unsupported option '--cstdlib=system' for target 'mmix-unknown-unknown'
// INVALID: error: invalid C library name in argument '--cstdlib=invalid'

int value;
