# RUN: llvm-mc -triple=mmix -filetype=obj %s -o %t
# RUN: llvm-readobj --sections --section-data --symbols --relocations \
# RUN:   --expand-relocs %t | FileCheck %s --check-prefix=OBJ \
# RUN:     --implicit-check-not=R_MMIX_CBRANCH \
# RUN:     --implicit-check-not=R_MMIX_PUSHJ \
# RUN:     --implicit-check-not=R_MMIX_JMP
# RUN: llvm-objdump --no-print-imm-hex -d %t \
# RUN:   | FileCheck %s --check-prefix=DIS --implicit-check-not='<unknown>'

.text
local_start:
BN r1, local_forward
JMP local_forward
local_forward:
BNB r2, local_start
JMPB local_start

.global undefined_global
BN r3, undefined_global + 4
BNB r4, undefined_global - 8
JMP undefined_global + 12
JMPB undefined_global - 16

.weak weak_undefined
GETA r5, weak_undefined + 20
GETAB r6, weak_undefined - 24

BN r7, .rodata + 28
JMP .rodata - 32
BN r8, local_other + 36
JMP local_other - 40

.set absolute_target, 64
GETA r9, absolute_target

BN r10, global_other + 44
JMP weak_defined - 48

.section .other,"ax",@progbits
local_other:
SWYM 0, 0, 0
.global global_other
global_other:
SWYM 0, 0, 0
.weak weak_defined
weak_defined:
SWYM 0, 0, 0

.section .rodata,"a",@progbits
SWYM 0, 0, 0

# Same-section and absolute expressions resolve without relocations. The
# unresolved fields remain zero while the instruction opcodes are preserved.
# OBJ:          Name: .text
# OBJ:          Size: 68
# OBJ:          SectionData (
# OBJ-NEXT:       0000: 40010002 F0000001 4102FFFE F1FFFFFD
# OBJ-NEXT:       0010: 40030000 41040000 F0000000 F1000000
# OBJ-NEXT:       0020: F4050000 F5060000 40070000 F0000000
# OBJ-NEXT:       0030: 40080000 F0000000 F4090040 400A0000
# OBJ-NEXT:       0040: F0000000
# OBJ-NEXT:     )

# Forward and backward spellings use the same direction-neutral relocation.
# RELA addends retain their signs, and local symbols in other sections are
# canonicalized to section symbols.
# OBJ:          Relocations [
# OBJ-NEXT:       Section ({{.*}}) .rela.text {
# OBJ-NEXT:         Relocation {
# OBJ-NEXT:           Offset: 0x10
# OBJ-NEXT:           Type: R_MMIX_ADDR19 (30)
# OBJ-NEXT:           Symbol: undefined_global
# OBJ-NEXT:           Addend: 0x4
# OBJ-NEXT:         }
# OBJ-NEXT:         Relocation {
# OBJ-NEXT:           Offset: 0x14
# OBJ-NEXT:           Type: R_MMIX_ADDR19 (30)
# OBJ-NEXT:           Symbol: undefined_global
# OBJ-NEXT:           Addend: 0xFFFFFFFFFFFFFFF8
# OBJ-NEXT:         }
# OBJ-NEXT:         Relocation {
# OBJ-NEXT:           Offset: 0x18
# OBJ-NEXT:           Type: R_MMIX_ADDR27 (31)
# OBJ-NEXT:           Symbol: undefined_global
# OBJ-NEXT:           Addend: 0xC
# OBJ-NEXT:         }
# OBJ-NEXT:         Relocation {
# OBJ-NEXT:           Offset: 0x1C
# OBJ-NEXT:           Type: R_MMIX_ADDR27 (31)
# OBJ-NEXT:           Symbol: undefined_global
# OBJ-NEXT:           Addend: 0xFFFFFFFFFFFFFFF0
# OBJ-NEXT:         }
# OBJ-NEXT:         Relocation {
# OBJ-NEXT:           Offset: 0x20
# OBJ-NEXT:           Type: R_MMIX_ADDR19 (30)
# OBJ-NEXT:           Symbol: weak_undefined
# OBJ-NEXT:           Addend: 0x14
# OBJ-NEXT:         }
# OBJ-NEXT:         Relocation {
# OBJ-NEXT:           Offset: 0x24
# OBJ-NEXT:           Type: R_MMIX_ADDR19 (30)
# OBJ-NEXT:           Symbol: weak_undefined
# OBJ-NEXT:           Addend: 0xFFFFFFFFFFFFFFE8
# OBJ-NEXT:         }
# OBJ-NEXT:         Relocation {
# OBJ-NEXT:           Offset: 0x28
# OBJ-NEXT:           Type: R_MMIX_ADDR19 (30)
# OBJ-NEXT:           Symbol: .rodata
# OBJ-NEXT:           Addend: 0x1C
# OBJ-NEXT:         }
# OBJ-NEXT:         Relocation {
# OBJ-NEXT:           Offset: 0x2C
# OBJ-NEXT:           Type: R_MMIX_ADDR27 (31)
# OBJ-NEXT:           Symbol: .rodata
# OBJ-NEXT:           Addend: 0xFFFFFFFFFFFFFFE0
# OBJ-NEXT:         }
# OBJ-NEXT:         Relocation {
# OBJ-NEXT:           Offset: 0x30
# OBJ-NEXT:           Type: R_MMIX_ADDR19 (30)
# OBJ-NEXT:           Symbol: .other
# OBJ-NEXT:           Addend: 0x24
# OBJ-NEXT:         }
# OBJ-NEXT:         Relocation {
# OBJ-NEXT:           Offset: 0x34
# OBJ-NEXT:           Type: R_MMIX_ADDR27 (31)
# OBJ-NEXT:           Symbol: .other
# OBJ-NEXT:           Addend: 0xFFFFFFFFFFFFFFD8
# OBJ-NEXT:         }
# OBJ-NEXT:         Relocation {
# OBJ-NEXT:           Offset: 0x3C
# OBJ-NEXT:           Type: R_MMIX_ADDR19 (30)
# OBJ-NEXT:           Symbol: global_other
# OBJ-NEXT:           Addend: 0x2C
# OBJ-NEXT:         }
# OBJ-NEXT:         Relocation {
# OBJ-NEXT:           Offset: 0x40
# OBJ-NEXT:           Type: R_MMIX_ADDR27 (31)
# OBJ-NEXT:           Symbol: weak_defined
# OBJ-NEXT:           Addend: 0xFFFFFFFFFFFFFFD0
# OBJ-NEXT:         }
# OBJ-NEXT:       }
# OBJ-NEXT:     ]

# Verify the bindings and definitions used by the relocation cases.
# OBJ:          Name: local_start
# OBJ:          Binding: Local
# OBJ:          Section: .text
# OBJ:          Name: absolute_target
# OBJ:          Value: 0x40
# OBJ:          Binding: Local
# OBJ:          Section: Absolute
# OBJ:          Name: .other
# OBJ:          Binding: Local
# OBJ:          Type: Section
# OBJ:          Section: .other
# OBJ:          Name: undefined_global
# OBJ:          Binding: Global
# OBJ:          Section: Undefined
# OBJ:          Name: weak_undefined
# OBJ:          Binding: Weak
# OBJ:          Section: Undefined
# OBJ:          Name: global_other
# OBJ:          Binding: Global
# OBJ:          Section: .other
# OBJ:          Name: weak_defined
# OBJ:          Binding: Weak
# OBJ:          Section: .other

# DIS-LABEL: <local_start>:
# DIS-NEXT:  0: 40 01 00 02   BN r1, 2
# DIS-NEXT:  4: f0 00 00 01   JMP 1
# DIS-LABEL: <local_forward>:
# DIS-NEXT:  8: 41 02 ff fe   BNB r2, -2
# DIS-NEXT:  c: f1 ff ff fd   JMPB -3
# DIS-NEXT:  10: 40 03 00 00   BN r3, 0
# DIS-NEXT:  14: 41 04 00 00   BNB r4, -65536
# DIS-NEXT:  18: f0 00 00 00   JMP 0
# DIS-NEXT:  1c: f1 00 00 00   JMPB -16777216
# DIS:       38: f4 09 00 40   GETA r9, 64
