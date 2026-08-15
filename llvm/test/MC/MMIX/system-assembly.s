# RUN: llvm-mc -triple=mmix -show-encoding %s \
# RUN:   | FileCheck %s --check-prefix=ENC
# RUN: llvm-mc -triple=mmix -filetype=obj %s -o %t
# RUN: llvm-readobj --file-headers --sections --symbols --relocations %t \
# RUN:   | FileCheck %s --check-prefix=OBJ
# RUN: llvm-objdump --no-print-imm-hex -dr %t \
# RUN:   | FileCheck %s --check-prefix=DIS --implicit-check-not='<unknown>'

# These independent snippets compose architectural system facilities without
# defining a trap, syscall, context-switch, or kernel ABI.

.text
.p2align 2
.global system_assembly
.type system_assembly,@function
system_assembly:
GET r1, rG
PUT rG, 231
PUT rL, 0
GETA r2, %geta(system_data)

GET r3, rV
PUT rV, r3
SYNC 6
LDVTS r4, r5, 0

GET r6, rK
PUT rK, r6
SYNC 3
CSWAP r7, r8, r9

TRIP 1, 2, 3
TRAP 0, 1, 2
RESUME 1

SAVE r9
UNSAVE r9

PUSHJ r10, external_handler
GO r11, r12, 0
POP 0, 0
.size system_assembly, .-system_assembly

.section .rodata.system,"a",@progbits
.p2align 3
.global system_data
.type system_data,@object
system_data:
.quad external_state + 8
.size system_data, .-system_data

.global external_handler
.type external_handler,@function
.global external_state
.type external_state,@object

# ENC: GET r1, rG{{.*}}[0xfe,0x01,0x00,0x13]
# ENC: PUT rG, 231{{.*}}[0xf7,0x13,0x00,0xe7]
# ENC: PUT rL, 0{{.*}}[0xf7,0x14,0x00,0x00]
# ENC: GETA r2, %geta(system_data)
# ENC-NEXT: fixup A - offset: 0, value: system_data, kind: fixup_mmix_geta
# ENC: GET r3, rV{{.*}}[0xfe,0x03,0x00,0x12]
# ENC: PUT rV, r3{{.*}}[0xf6,0x12,0x00,0x03]
# ENC: SYNC 6{{.*}}[0xfc,0x00,0x00,0x06]
# ENC: LDVTS r4, r5, 0{{.*}}[0x99,0x04,0x05,0x00]
# ENC: GET r6, rK{{.*}}[0xfe,0x06,0x00,0x0f]
# ENC: PUT rK, r6{{.*}}[0xf6,0x0f,0x00,0x06]
# ENC: SYNC 3{{.*}}[0xfc,0x00,0x00,0x03]
# ENC: CSWAP r7, r8, r9{{.*}}[0x94,0x07,0x08,0x09]
# ENC: TRIP 1, 2, 3{{.*}}[0xff,0x01,0x02,0x03]
# ENC: TRAP 0, 1, 2{{.*}}[0x00,0x00,0x01,0x02]
# ENC: RESUME 1{{.*}}[0xf9,0x00,0x00,0x01]
# ENC: SAVE r9{{.*}}[0xfa,0x09,0x00,0x00]
# ENC: UNSAVE r9{{.*}}[0xfb,0x00,0x00,0x09]
# ENC: PUSHJ r10, external_handler
# ENC-NEXT: fixup A - offset: 0, value: external_handler, kind: fixup_mmix_call
# ENC: GO r11, r12, 0{{.*}}[0x9f,0x0b,0x0c,0x00]
# ENC: POP 0, 0{{.*}}[0xf8,0x00,0x00,0x00]

# OBJ: Format: elf64-mmix
# OBJ-NEXT: Arch: mmix
# OBJ: Type: Relocatable
# OBJ-NEXT: Machine: EM_MMIX
# OBJ: Name: .text
# OBJ: AddressAlignment: 4
# OBJ: Name: .rodata.system
# OBJ: AddressAlignment: 8
# OBJ: Relocations [
# OBJ: Section ({{.*}}) .rela.text {
# OBJ-NEXT: 0xC R_MMIX_GETA system_data 0x0
# OBJ-NEXT: 0x50 R_MMIX_PUSHJ_STUBBABLE external_handler 0x0
# OBJ: Section ({{.*}}) .rela.rodata.system {
# OBJ-NEXT: 0x0 R_MMIX_64 external_state 0x8
# OBJ: Name: system_assembly
# OBJ: Size: 92
# OBJ: Type: Function
# OBJ: Name: system_data
# OBJ: Size: 8
# OBJ: Type: Object

# DIS-LABEL: <system_assembly>:
# DIS: GET r1, rG
# DIS-NEXT: PUT rG, 231
# DIS-NEXT: PUT rL, 0
# DIS: GETA r2, 0
# DIS: GET r3, rV
# DIS-NEXT: PUT rV, r3
# DIS-NEXT: SYNC 6
# DIS-NEXT: LDVTS r4, r5, 0
# DIS: GET r6, rK
# DIS-NEXT: PUT rK, r6
# DIS-NEXT: SYNC 3
# DIS-NEXT: CSWAP r7, r8, r9
# DIS: TRIP 1, 2, 3
# DIS-NEXT: TRAP 0, 1, 2
# DIS-NEXT: RESUME 1
# DIS: SAVE r9
# DIS-NEXT: UNSAVE r9
# DIS: PUSHJ r10, 0
# DIS: R_MMIX_PUSHJ_STUBBABLE external_handler
# DIS-NEXT: {{.*}} GO r11, r12, 0
# DIS-NEXT: POP 0, 0
