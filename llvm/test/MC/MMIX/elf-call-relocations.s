# RUN: llvm-mc -triple=mmix -filetype=obj %s -o %t
# RUN: llvm-readobj --sections --section-data --symbols --relocations \
# RUN:   --expand-relocs %t | FileCheck %s --check-prefix=OBJ \
# RUN:     --implicit-check-not='R_MMIX_PUSHJ (22)' \
# RUN:     --implicit-check-not=R_MMIX_ADDR19 \
# RUN:     --implicit-check-not=R_MMIX_GETA
# RUN: llvm-objdump --no-print-imm-hex -d %t \
# RUN:   | FileCheck %s --check-prefix=DIS --implicit-check-not='<unknown>'

.text
.type same_start,@function
same_start:
PUSHJ r1, same_forward
.type same_forward,@function
same_forward:
PUSHJB r2, same_start

.global undefined_global
.type undefined_global,@function
PUSHJ r3, undefined_global + 4
PUSHJB r4, undefined_global - 8

.weak weak_undefined
.type weak_undefined,@function
PUSHJ r5, weak_undefined + 12
PUSHJB r6, weak_undefined - 16

PUSHJ r7, local_other + 20
PUSHJB r8, global_other - 24
PUSHJ r9, weak_defined + 28
PUSHJB r10, exported_text - 32

.global exported_text
.type exported_text,@function
exported_text:
SWYM 0, 0, 0

.section .other,"ax",@progbits
.type local_other,@function
local_other:
SWYM 0, 0, 0
.global global_other
.type global_other,@function
global_other:
SWYM 0, 0, 0
.weak weak_defined
.type weak_defined,@function
weak_defined:
SWYM 0, 0, 0

# Same-section calls resolve directly. Unresolved calls retain their original
# opcode and X field while leaving the displacement field for the linker.
# OBJ:          Name: .text
# OBJ:          Size: 44
# OBJ:          SectionData (
# OBJ-NEXT:       0000: F2010001 F302FFFF F2030000 F3040000
# OBJ-NEXT:       0010: F2050000 F3060000 F2070000 F3080000
# OBJ-NEXT:       0020: F2090000 F30A0000 FD000000
# OBJ-NEXT:     )

# OBJ:          Relocations [
# OBJ-NEXT:       Section ({{.*}}) .rela.text {
# OBJ-NEXT:         Relocation {
# OBJ-NEXT:           Offset: 0x8
# OBJ-NEXT:           Type: R_MMIX_PUSHJ_STUBBABLE (36)
# OBJ-NEXT:           Symbol: undefined_global
# OBJ-NEXT:           Addend: 0x4
# OBJ-NEXT:         }
# OBJ-NEXT:         Relocation {
# OBJ-NEXT:           Offset: 0xC
# OBJ-NEXT:           Type: R_MMIX_PUSHJ_STUBBABLE (36)
# OBJ-NEXT:           Symbol: undefined_global
# OBJ-NEXT:           Addend: 0xFFFFFFFFFFFFFFF8
# OBJ-NEXT:         }
# OBJ-NEXT:         Relocation {
# OBJ-NEXT:           Offset: 0x10
# OBJ-NEXT:           Type: R_MMIX_PUSHJ_STUBBABLE (36)
# OBJ-NEXT:           Symbol: weak_undefined
# OBJ-NEXT:           Addend: 0xC
# OBJ-NEXT:         }
# OBJ-NEXT:         Relocation {
# OBJ-NEXT:           Offset: 0x14
# OBJ-NEXT:           Type: R_MMIX_PUSHJ_STUBBABLE (36)
# OBJ-NEXT:           Symbol: weak_undefined
# OBJ-NEXT:           Addend: 0xFFFFFFFFFFFFFFF0
# OBJ-NEXT:         }
# OBJ-NEXT:         Relocation {
# OBJ-NEXT:           Offset: 0x18
# OBJ-NEXT:           Type: R_MMIX_PUSHJ_STUBBABLE (36)
# OBJ-NEXT:           Symbol: .other
# OBJ-NEXT:           Addend: 0x14
# OBJ-NEXT:         }
# OBJ-NEXT:         Relocation {
# OBJ-NEXT:           Offset: 0x1C
# OBJ-NEXT:           Type: R_MMIX_PUSHJ_STUBBABLE (36)
# OBJ-NEXT:           Symbol: global_other
# OBJ-NEXT:           Addend: 0xFFFFFFFFFFFFFFE8
# OBJ-NEXT:         }
# OBJ-NEXT:         Relocation {
# OBJ-NEXT:           Offset: 0x20
# OBJ-NEXT:           Type: R_MMIX_PUSHJ_STUBBABLE (36)
# OBJ-NEXT:           Symbol: weak_defined
# OBJ-NEXT:           Addend: 0x1C
# OBJ-NEXT:         }
# OBJ-NEXT:         Relocation {
# OBJ-NEXT:           Offset: 0x24
# OBJ-NEXT:           Type: R_MMIX_PUSHJ_STUBBABLE (36)
# OBJ-NEXT:           Symbol: exported_text
# OBJ-NEXT:           Addend: 0xFFFFFFFFFFFFFFE0
# OBJ-NEXT:         }
# OBJ-NEXT:       }
# OBJ-NEXT:     ]

# Local cross-section calls use a section symbol. Global and weak function
# symbols retain their ELF binding and definition state.
# OBJ:          Name: .other
# OBJ:          Binding: Local
# OBJ:          Type: Section
# OBJ:          Section: .other
# OBJ:          Name: undefined_global
# OBJ:          Binding: Global
# OBJ:          Type: Function
# OBJ:          Section: Undefined
# OBJ:          Name: weak_undefined
# OBJ:          Binding: Weak
# OBJ:          Type: Function
# OBJ:          Section: Undefined
# OBJ:          Name: global_other
# OBJ:          Binding: Global
# OBJ:          Type: Function
# OBJ:          Section: .other
# OBJ:          Name: weak_defined
# OBJ:          Binding: Weak
# OBJ:          Type: Function
# OBJ:          Section: .other
# OBJ:          Name: exported_text
# OBJ:          Binding: Global
# OBJ:          Type: Function
# OBJ:          Section: .text

# DIS-LABEL: <same_start>:
# DIS-NEXT:  0: f2 01 00 01   PUSHJ r1, 1
# DIS-LABEL: <same_forward>:
# DIS-NEXT:  4: f3 02 ff ff   PUSHJB r2, -1
# DIS-NEXT:  8: f2 03 00 00   PUSHJ r3, 0
# DIS-NEXT:  c: f3 04 00 00   PUSHJB r4, -65536
# DIS-NEXT:  10: f2 05 00 00   PUSHJ r5, 0
# DIS-NEXT:  14: f3 06 00 00   PUSHJB r6, -65536
