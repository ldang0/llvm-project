// RUN: %clang -### --target=mmix -ffreestanding -E \
// RUN:   %S/Inputs/freestanding.c -o %t.i 2>&1 \
// RUN:   | FileCheck %s --check-prefix=FRONTEND -DTRIPLE=mmix \
// RUN:       --implicit-check-not=-internal-isystem \
// RUN:       --implicit-check-not=-internal-externc-isystem \
// RUN:       --implicit-check-not=-isysroot --implicit-check-not=-emit-llvm \
// RUN:       --implicit-check-not=-emit-obj --implicit-check-not=-cc1as
// RUN: %clang -### --target=mmix-unknown-unknown -ffreestanding -E \
// RUN:   %S/Inputs/freestanding.c -o %t.i 2>&1 \
// RUN:   | FileCheck %s --check-prefix=FRONTEND \
// RUN:       -DTRIPLE=mmix-unknown-unknown \
// RUN:       --implicit-check-not=-internal-isystem \
// RUN:       --implicit-check-not=-internal-externc-isystem \
// RUN:       --implicit-check-not=-isysroot --implicit-check-not=-emit-llvm \
// RUN:       --implicit-check-not=-emit-obj --implicit-check-not=-cc1as
// RUN: %clang -### --target=mmix-none-none -ffreestanding -E \
// RUN:   %S/Inputs/freestanding.c -o %t.i 2>&1 \
// RUN:   | FileCheck %s --check-prefix=FRONTEND -DTRIPLE=mmix-none-none \
// RUN:       --implicit-check-not=-internal-isystem \
// RUN:       --implicit-check-not=-internal-externc-isystem \
// RUN:       --implicit-check-not=-isysroot --implicit-check-not=-emit-llvm \
// RUN:       --implicit-check-not=-emit-obj --implicit-check-not=-cc1as
// RUN: %clang -### --target=mmix-unknown-none-none -ffreestanding -E \
// RUN:   %S/Inputs/freestanding.c -o %t.i 2>&1 \
// RUN:   | FileCheck %s --check-prefix=FRONTEND \
// RUN:       -DTRIPLE=mmix-unknown-none-none \
// RUN:       --implicit-check-not=-internal-isystem \
// RUN:       --implicit-check-not=-internal-externc-isystem \
// RUN:       --implicit-check-not=-isysroot --implicit-check-not=-emit-llvm \
// RUN:       --implicit-check-not=-emit-obj --implicit-check-not=-cc1as
// RUN: %clang -### --target=mmix-unknown-unknown -ffreestanding \
// RUN:   -fsyntax-only %S/Inputs/freestanding.c 2>&1 \
// RUN:   | FileCheck %s --check-prefix=SYNTAX \
// RUN:       --implicit-check-not=-internal-isystem \
// RUN:       --implicit-check-not=-internal-externc-isystem \
// RUN:       --implicit-check-not=-isysroot --implicit-check-not=-emit-llvm \
// RUN:       --implicit-check-not=-emit-obj --implicit-check-not=-cc1as

// RUN: %clang --target=mmix -ffreestanding -E \
// RUN:   %S/Inputs/freestanding.c -o %t.i
// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -fsyntax-only \
// RUN:   %S/Inputs/freestanding.c

// RUN: not %clang --target=mmix-pc-unknown -ffreestanding -fsyntax-only \
// RUN:   %S/Inputs/freestanding.c 2>&1 \
// RUN:   | FileCheck %s --check-prefix=BAD-VENDOR
// RUN: not %clang --target=mmix-unknown-freebsd -ffreestanding -fsyntax-only \
// RUN:   %S/Inputs/freestanding.c 2>&1 \
// RUN:   | FileCheck %s --check-prefix=BAD-OS
// RUN: not %clang --target=mmix-unknown-unknown-elf -ffreestanding \
// RUN:   -fsyntax-only %S/Inputs/freestanding.c 2>&1 \
// RUN:   | FileCheck %s --check-prefix=BAD-ENV

// FRONTEND: (in-process)
// FRONTEND-NEXT: {{.*}}clang{{.*}} "-cc1" "-triple" "[[TRIPLE]]"
// FRONTEND-SAME: "-E"
// FRONTEND-SAME: "-ffreestanding"
// FRONTEND-SAME: "-o" "{{.*}}.i"
// FRONTEND-SAME: "-x" "c" "{{.*}}Inputs{{/|\\}}freestanding.c"

// SYNTAX: (in-process)
// SYNTAX-NEXT: {{.*}}clang{{.*}} "-cc1" "-triple" "mmix-unknown-unknown"
// SYNTAX-SAME: "-fsyntax-only"
// SYNTAX-SAME: "-ffreestanding"
// SYNTAX-SAME: "-x" "c" "{{.*}}Inputs{{/|\\}}freestanding.c"

// BAD-VENDOR: error: unknown target triple 'mmix-pc-unknown'
// BAD-OS: error: unknown target triple 'mmix-unknown-freebsd'
// BAD-ENV: error: unknown target triple 'mmix-unknown-unknown-elf'
