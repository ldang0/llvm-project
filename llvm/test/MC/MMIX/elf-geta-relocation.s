# RUN: llvm-mc -triple=mmix -filetype=obj %s -o %t
# RUN: llvm-readobj --sections --section-data --symbols --relocations \
# RUN:   --expand-relocs %t | FileCheck %s --check-prefix=OBJ \
# RUN:     --implicit-check-not=R_MMIX_GETA_
# RUN: llvm-objdump --no-print-imm-hex -d %t \
# RUN:   | FileCheck %s --check-prefix=DIS --implicit-check-not='<unknown>'

.text
local_start:
GETA r2, %geta(local_forward)
local_forward:
GETA r3, %geta(local_start)
relocation_site:
GETA r7, %geta(external - 9)
after_reservation:
SWYM 1, 2, 3

# OBJ:      Name: .text
# OBJ-NEXT: Type: SHT_PROGBITS (0x1)
# OBJ:      Size: 28
# OBJ:      AddressAlignment: 4
# OBJ-NEXT: EntrySize: 0
# OBJ-NEXT: SectionData (
# OBJ-NEXT:   0000: F4020001 F503FFFF F4070000 FD000000
# OBJ-NEXT:   0010: FD000000 FD000000 FD010203
# OBJ-NEXT: )

# OBJ:      Name: .rela.text
# OBJ-NEXT: Type: SHT_RELA (0x4)
# OBJ:      Size: 24
# OBJ:      AddressAlignment: 8
# OBJ-NEXT: EntrySize: 24

# OBJ:      Relocations [
# OBJ-NEXT:   Section ({{[0-9]+}}) .rela.text {
# OBJ-NEXT:     Relocation {
# OBJ-NEXT:       Offset: 0x8
# OBJ-NEXT:       Type: R_MMIX_GETA (13)
# OBJ-NEXT:       Symbol: external ([[EXTERNAL:[0-9]+]])
# OBJ-NEXT:       Addend: 0xFFFFFFFFFFFFFFF7
# OBJ-NEXT:     }
# OBJ-NEXT:   }
# OBJ-NEXT: ]

# OBJ:      Name: after_reservation
# OBJ-NEXT: Value: 0x18
# OBJ:      Name: external
# OBJ-NEXT: Value: 0x0
# OBJ-NEXT: Size: 0
# OBJ-NEXT: Binding: Global (0x1)
# OBJ-NEXT: Type: None (0x0)
# OBJ-NEXT: Other: 0
# OBJ-NEXT: Section: Undefined (0x0)

# DIS:      <local_start>:
# DIS-NEXT: {{.*}} GETA r2, 1
# DIS:      <local_forward>:
# DIS-NEXT: {{.*}} GETAB r3, -1
# DIS:      <relocation_site>:
# DIS-NEXT: {{.*}} GETA r7, 0
# DIS-NEXT: {{.*}} SWYM 0, 0, 0
# DIS-NEXT: {{.*}} SWYM 0, 0, 0
# DIS-NEXT: {{.*}} SWYM 0, 0, 0
# DIS:      <after_reservation>:
# DIS-NEXT: {{.*}} SWYM 1, 2, 3
