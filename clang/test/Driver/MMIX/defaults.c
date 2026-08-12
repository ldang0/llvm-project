// RUN: %clang -### --target=mmix-unknown-unknown -ffreestanding -E %s \
// RUN:   -o %t.i 2>&1 | FileCheck %s --check-prefix=PREPROCESS-JOB
// RUN: %clang -### --target=mmix-unknown-unknown -ffreestanding \
// RUN:   -fsyntax-only %s 2>&1 | FileCheck %s --check-prefix=SYNTAX-JOB
// RUN: %clang -### --target=mmix-unknown-unknown -ffreestanding \
// RUN:   -S -emit-llvm %s -o %t.ll 2>&1 \
// RUN:   | FileCheck %s --check-prefix=IR-JOB --implicit-check-not=-target-cpu \
// RUN:       --implicit-check-not=-target-feature --implicit-check-not=-mabi \
// RUN:       --implicit-check-not=-mcode-model
// RUN: %clang -### --target=mmix-unknown-unknown -ffreestanding \
// RUN:   -c -emit-llvm %s -o %t.bc 2>&1 \
// RUN:   | FileCheck %s --check-prefix=BC-JOB --implicit-check-not=-target-cpu \
// RUN:       --implicit-check-not=-target-feature --implicit-check-not=-mabi \
// RUN:       --implicit-check-not=-mcode-model
// RUN: %clang -### --target=mmix-unknown-unknown -ffreestanding -S %s \
// RUN:   -o %t.s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ASM-JOB --implicit-check-not=-target-cpu \
// RUN:       --implicit-check-not=-target-feature --implicit-check-not=-mabi \
// RUN:       --implicit-check-not=-mcode-model \
// RUN:       --implicit-check-not=-output-asm-variant
// RUN: %clang -### --target=mmix-unknown-unknown -ffreestanding -c %s \
// RUN:   -o %t.o 2>&1 \
// RUN:   | FileCheck %s --check-prefix=OBJECT-JOB \
// RUN:       --implicit-check-not=-cc1as --implicit-check-not=-target-cpu \
// RUN:       --implicit-check-not=-target-feature --implicit-check-not=-mabi \
// RUN:       --implicit-check-not=-mcode-model
// RUN: %clang -### --target=mmix-unknown-unknown \
// RUN:   -c %S/Inputs/canonical.s -o %t-assembly.o 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ASSEMBLER-JOB

// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -dM -E %s \
// RUN:   | FileCheck %s --check-prefix=MACROS
// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -S -emit-llvm %s \
// RUN:   -o - | FileCheck %s --check-prefix=IR
// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -S %s -o %t.s
// RUN: FileCheck %s --check-prefix=ASM --implicit-check-not=MMIXAL \
// RUN:   --implicit-check-not=GREG --implicit-check-not=Main \
// RUN:   --implicit-check-not=OCTA --implicit-check-not=BSPEC < %t.s
// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -c %s -o %t.o
// RUN: llvm-readobj --file-headers %t.o \
// RUN:   | FileCheck %s --check-prefix=ELF

// RUN: not %clang --target=mmix-unknown-unknown -mabi=gnu \
// RUN:   -fsyntax-only %s 2>&1 | FileCheck %s --check-prefix=ABI-OPTION
// RUN: not %clang --target=mmix-unknown-unknown -mcpu=generic \
// RUN:   -fsyntax-only %s 2>&1 | FileCheck %s --check-prefix=CPU-OPTION
// RUN: not %clang --target=mmix-unknown-unknown -mcmodel=small \
// RUN:   -fsyntax-only %s 2>&1 | FileCheck %s --check-prefix=MODEL-OPTION
// RUN: not %clang --target=mmix-unknown-unknown -mattr=+base \
// RUN:   -fsyntax-only %s 2>&1 | FileCheck %s --check-prefix=FEATURE-OPTION

// PREPROCESS-JOB: (in-process)
// PREPROCESS-JOB-NEXT: {{.*}}clang{{.*}} "-cc1" "-triple" "mmix-unknown-unknown"
// PREPROCESS-JOB-SAME: "-E"

// SYNTAX-JOB: (in-process)
// SYNTAX-JOB-NEXT: {{.*}}clang{{.*}} "-cc1" "-triple" "mmix-unknown-unknown"
// SYNTAX-JOB-SAME: "-fsyntax-only"

// IR-JOB: (in-process)
// IR-JOB-NEXT: {{.*}}clang{{.*}} "-cc1" "-triple" "mmix-unknown-unknown"
// IR-JOB-SAME: "-emit-llvm"
// IR-JOB-SAME: "-mrelocation-model" "static"

// BC-JOB: (in-process)
// BC-JOB-NEXT: {{.*}}clang{{.*}} "-cc1" "-triple" "mmix-unknown-unknown"
// BC-JOB-SAME: "-emit-llvm-bc"
// BC-JOB-SAME: "-mrelocation-model" "static"

// ASM-JOB: (in-process)
// ASM-JOB-NEXT: {{.*}}clang{{.*}} "-cc1" "-triple" "mmix-unknown-unknown"
// ASM-JOB-SAME: "-S"
// ASM-JOB-SAME: "-mrelocation-model" "static"

// OBJECT-JOB: (in-process)
// OBJECT-JOB-NEXT: {{.*}}clang{{.*}} "-cc1" "-triple" "mmix-unknown-unknown"
// OBJECT-JOB-SAME: "-emit-obj"
// OBJECT-JOB-SAME: "-mrelocation-model" "static"

// ASSEMBLER-JOB: (in-process)
// ASSEMBLER-JOB-NEXT: {{.*}}clang{{.*}} "-cc1as" "-triple" "mmix-unknown-unknown"
// ASSEMBLER-JOB-SAME: "-filetype" "obj"
// ASSEMBLER-JOB-SAME: "-mrelocation-model" "static"

// MACROS: #define __MMIX_ABI_GNU__ 1
// MACROS: #define __MMIX__ 1

// IR: target datalayout = "E-m:e-p:64:64-i64:64-n64-S64"
// IR-NEXT: target triple = "mmix-unknown-unknown"
// IR: attributes #{{[0-9]+}} = {
// IR-SAME: "target-features"="+base,+cache,+system,+virtual-memory"

// ASM: .text
// ASM: .globl fixed_defaults
// ASM: .type fixed_defaults,@function
// ASM-LABEL: fixed_defaults:
// ASM: ADDU r231, r{{[0-9]+}}, 1
// ASM: POP 0, 0

// ELF: Format: elf64-mmix
// ELF-NEXT: Arch: mmix
// ELF: Class: 64-bit
// ELF: DataEncoding: BigEndian
// ELF: Type: Relocatable
// ELF: Machine: EM_MMIX

// ABI-OPTION: error: unsupported option '-mabi=' for target 'mmix-unknown-unknown'
// CPU-OPTION: error: unsupported option '-mcpu=' for target 'mmix-unknown-unknown'
// MODEL-OPTION: error: unsupported argument 'small' to option '-mcmodel=' for target 'mmix-unknown-unknown'
// FEATURE-OPTION: error: unknown argument: '-mattr=+base'

long fixed_defaults(long value) { return value + 1; }
