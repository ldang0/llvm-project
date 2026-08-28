// RUN: %clang -### --target=mmix-unknown-unknown -ffreestanding -std=gnu2x \
// RUN:   -S %S/../../CodeGen/mmix-abi-backend-integration.c -o %t.s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=JOB \
// RUN:       --implicit-check-not=-cc1as --implicit-check-not=-emit-llvm \
// RUN:       --implicit-check-not=-emit-obj \
// RUN:       --implicit-check-not=-target-cpu \
// RUN:       --implicit-check-not=-target-feature \
// RUN:       --implicit-check-not=-mcode-model \
// RUN:       --implicit-check-not=-output-asm-variant

// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -std=gnu2x \
// RUN:   -S %S/../../CodeGen/mmix-abi-backend-integration.c -o %t.s
// RUN: FileCheck %s --check-prefix=ASM --implicit-check-not=MMIXAL \
// RUN:   --implicit-check-not=GREG --implicit-check-not='LOC #' \
// RUN:   --implicit-check-not=Main --implicit-check-not='{{\$[0-9]+}}' \
// RUN:   --implicit-check-not='PUT rA' --implicit-check-not='PUT rL' \
// RUN:   --implicit-check-not=OCTA --implicit-check-not=BYTE \
// RUN:   --implicit-check-not=BSPEC --implicit-check-not=ESPEC < %t.s

// JOB: (in-process)
// JOB-NEXT: {{.*}}clang{{.*}} "-cc1" "-triple" "mmix-unknown-unknown"
// JOB-SAME: "-S"
// JOB-SAME: "-mrelocation-model" "static"
// JOB-SAME: "-ffreestanding"
// JOB-SAME: "-std=gnu2x"
// JOB-SAME: "-o" "{{.*}}.s"
// JOB-SAME: "-x" "c" "{{.*}}mmix-abi-backend-integration.c"

// ASM: .text
// ASM: .globl compose
// ASM: .type compose,@function
// ASM-LABEL: compose:
// ASM: GET r30, rJ
// ASM: PUSHJ r31, recursive_sum
// ASM: PUSHGO r31, r{{[0-9]+}}, 0
// ASM: GETA r{{[0-9]+}}, %geta(external_scalar)
// ASM: POP 0, 0

// ASM-LABEL: recursive_sum:
// ASM: PUSHJB r31, recursive_sum
// ASM: POP 0, 0

// ASM-LABEL: copy_block:
// ASM: GETA r{{[0-9]+}}, %geta(memcpy)
// ASM: PUSHGO r31, r{{[0-9]+}}, 0
// ASM: POP 0, 0

// ASM: .data
// ASM: .globl global_seed
// ASM-LABEL: global_seed:
// ASM-NEXT: .8byte 7
// ASM: .globl global_pointer
// ASM-LABEL: global_pointer:
// ASM-NEXT: .8byte global_seed
