# RUN: llvm-mc -triple=mmix -filetype=obj %s -o %t
# RUN: llvm-readobj --sections --section-data --symbols --relocations \
# RUN:   --expand-relocs %t | FileCheck %s --check-prefix=OBJ \
# RUN:     --implicit-check-not=R_MMIX_GETA_
# RUN: llvm-objdump --no-print-imm-hex -d %t \
# RUN:   | FileCheck %s --check-prefix=DIS --implicit-check-not='<unknown>'

.text
.global global_before
global_before:
local_start:
GETA r1, %geta(local_forward + 4)
local_forward:
GETA r2, %geta(local_start - 4)
GETA r3, %geta(absolute_target)

# S + A - P is -4 with the input layout.
GETA r4, %geta(global_before + 8)

.weak weak_defined
weak_defined:
GETA r5, %geta(weak_defined - 4)

GETA r6, %geta(undefined_global + 12)
GETA r7, %geta(undefined_global - 16)
GETA r8, %geta(.rodata + 20)

.weak weak_undefined
GETA r9, %geta(weak_undefined + 24)

# S + A - P is +12 with the input layout.
GETA r10, %geta(global_after - 8)

after_reservations:
SWYM 1, 2, 3

.global global_after
global_after:
SWYM 0, 0, 0

# Keep the symbolic expression until MC fixup evaluation, then resolve it as
# an absolute target without a relocation.
.set absolute_target, 64

.section .rodata,"a",@progbits
SWYM 0, 0, 0

# OBJ:      Index: [[TEXT:2]]
# OBJ-NEXT: Name: .text
# OBJ-NEXT: Type: SHT_PROGBITS (0x1)
# OBJ:      Size: 132
# OBJ:      AddressAlignment: 4
# OBJ-NEXT: EntrySize: 0
# OBJ-NEXT: SectionData (
# OBJ-NEXT:   0000: F4010002 F502FFFE F403000E F4040000
# OBJ-NEXT:   0010: FD000000 FD000000 FD000000 F4050000
# OBJ-NEXT:   0020: FD000000 FD000000 FD000000 F4060000
# OBJ-NEXT:   0030: FD000000 FD000000 FD000000 F4070000
# OBJ-NEXT:   0040: FD000000 FD000000 FD000000 F4080000
# OBJ-NEXT:   0050: FD000000 FD000000 FD000000 F4090000
# OBJ-NEXT:   0060: FD000000 FD000000 FD000000 F40A0000
# OBJ-NEXT:   0070: FD000000 FD000000 FD000000 FD010203
# OBJ-NEXT:   0080: FD000000
# OBJ-NEXT: )

# OBJ:      Index: [[RELA:3]]
# OBJ-NEXT: Name: .rela.text
# OBJ-NEXT: Type: SHT_RELA (0x4)
# OBJ:      Size: 168
# OBJ:      Link: [[SYMTAB:5]]
# OBJ-NEXT: Info: [[TEXT]]
# OBJ:      AddressAlignment: 8
# OBJ-NEXT: EntrySize: 24

# OBJ:      Index: [[SYMTAB]]
# OBJ-NEXT: Name: .symtab

# OBJ:      Relocations [
# OBJ-NEXT:   Section ([[RELA]]) .rela.text {
# OBJ-NEXT:     Relocation {
# OBJ-NEXT:       Offset: 0xC
# OBJ-NEXT:       Type: R_MMIX_GETA (13)
# OBJ-NEXT:       Symbol: global_before (6)
# OBJ-NEXT:       Addend: 0x8
# OBJ-NEXT:     }
# OBJ-NEXT:     Relocation {
# OBJ-NEXT:       Offset: 0x1C
# OBJ-NEXT:       Type: R_MMIX_GETA (13)
# OBJ-NEXT:       Symbol: weak_defined (7)
# OBJ-NEXT:       Addend: 0xFFFFFFFFFFFFFFFC
# OBJ-NEXT:     }
# OBJ-NEXT:     Relocation {
# OBJ-NEXT:       Offset: 0x2C
# OBJ-NEXT:       Type: R_MMIX_GETA (13)
# OBJ-NEXT:       Symbol: undefined_global (8)
# OBJ-NEXT:       Addend: 0xC
# OBJ-NEXT:     }
# OBJ-NEXT:     Relocation {
# OBJ-NEXT:       Offset: 0x3C
# OBJ-NEXT:       Type: R_MMIX_GETA (13)
# OBJ-NEXT:       Symbol: undefined_global (8)
# OBJ-NEXT:       Addend: 0xFFFFFFFFFFFFFFF0
# OBJ-NEXT:     }
# OBJ-NEXT:     Relocation {
# OBJ-NEXT:       Offset: 0x4C
# OBJ-NEXT:       Type: R_MMIX_GETA (13)
# OBJ-NEXT:       Symbol: .rodata (4)
# OBJ-NEXT:       Addend: 0x14
# OBJ-NEXT:     }
# OBJ-NEXT:     Relocation {
# OBJ-NEXT:       Offset: 0x5C
# OBJ-NEXT:       Type: R_MMIX_GETA (13)
# OBJ-NEXT:       Symbol: weak_undefined (9)
# OBJ-NEXT:       Addend: 0x18
# OBJ-NEXT:     }
# OBJ-NEXT:     Relocation {
# OBJ-NEXT:       Offset: 0x6C
# OBJ-NEXT:       Type: R_MMIX_GETA (13)
# OBJ-NEXT:       Symbol: global_after (10)
# OBJ-NEXT:       Addend: 0xFFFFFFFFFFFFFFF8
# OBJ-NEXT:     }
# OBJ-NEXT:   }
# OBJ-NEXT: ]

# OBJ:      Name: local_forward
# OBJ-NEXT: Value: 0x4
# OBJ-NEXT: Size: 0
# OBJ-NEXT: Binding: Local (0x0)
# OBJ-NEXT: Type: None (0x0)
# OBJ-NEXT: Other: 0
# OBJ-NEXT: Section: .text
# OBJ:      Name: absolute_target
# OBJ-NEXT: Value: 0x40
# OBJ-NEXT: Size: 0
# OBJ-NEXT: Binding: Local (0x0)
# OBJ-NEXT: Type: None (0x0)
# OBJ-NEXT: Other: 0
# OBJ-NEXT: Section: Absolute (0xFFF1)
# OBJ:      Name: after_reservations
# OBJ-NEXT: Value: 0x7C
# OBJ:      Name: global_before
# OBJ-NEXT: Value: 0x0
# OBJ-NEXT: Size: 0
# OBJ-NEXT: Binding: Global (0x1)
# OBJ-NEXT: Type: None (0x0)
# OBJ-NEXT: Other: 0
# OBJ-NEXT: Section: .text
# OBJ:      Name: weak_defined
# OBJ-NEXT: Value: 0x1C
# OBJ-NEXT: Size: 0
# OBJ-NEXT: Binding: Weak (0x2)
# OBJ-NEXT: Type: None (0x0)
# OBJ-NEXT: Other: 0
# OBJ-NEXT: Section: .text
# OBJ:      Name: undefined_global
# OBJ-NEXT: Value: 0x0
# OBJ-NEXT: Size: 0
# OBJ-NEXT: Binding: Global (0x1)
# OBJ-NEXT: Type: None (0x0)
# OBJ-NEXT: Other: 0
# OBJ-NEXT: Section: Undefined (0x0)
# OBJ:      Name: weak_undefined
# OBJ-NEXT: Value: 0x0
# OBJ-NEXT: Size: 0
# OBJ-NEXT: Binding: Weak (0x2)
# OBJ-NEXT: Type: None (0x0)
# OBJ-NEXT: Other: 0
# OBJ-NEXT: Section: Undefined (0x0)
# OBJ:      Name: global_after
# OBJ-NEXT: Value: 0x80
# OBJ-NEXT: Size: 0
# OBJ-NEXT: Binding: Global (0x1)
# OBJ-NEXT: Type: None (0x0)
# OBJ-NEXT: Other: 0
# OBJ-NEXT: Section: .text
# DIS:      <local_start>:
# DIS-NEXT: {{.*}} GETA r1, 2
# DIS:      <local_forward>:
# DIS-NEXT: {{.*}} GETAB r2, -2
# DIS-NEXT: {{.*}} GETA r3, 14
# DIS-NEXT: {{.*}} GETA r4, 0
# DIS:      <weak_defined>:
# DIS-NEXT: {{.*}} GETA r5, 0
# DIS:      {{.*}} GETA r6, 0
# DIS:      {{.*}} GETA r7, 0
# DIS:      {{.*}} GETA r8, 0
# DIS:      {{.*}} GETA r9, 0
# DIS:      {{.*}} GETA r10, 0
# DIS:      <after_reservations>:
# DIS-NEXT: {{.*}} SWYM 1, 2, 3
# DIS:      <global_after>:
# DIS-NEXT: {{.*}} SWYM 0, 0, 0
