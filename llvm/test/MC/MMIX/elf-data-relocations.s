# RUN: llvm-mc -triple=mmix -filetype=obj %s -o %t
# RUN: llvm-readobj --sections --section-data --relocations --expand-relocs %t \
# RUN:   | FileCheck %s

.data
.global external
.byte external + 1
.p2align 1
.short external - 2
.p2align 2
.long external + 3
.p2align 3
.quad external - 4

# CHECK:      Name: .text
# CHECK:      Index: [[DATA:[0-9]+]]
# CHECK-NEXT: Name: .data
# CHECK-NEXT: Type: SHT_PROGBITS (0x1)
# CHECK:      Size: 16
# CHECK:      AddressAlignment: 8
# CHECK-NEXT: EntrySize: 0
# CHECK-NEXT: SectionData (
# CHECK-NEXT:   0000: 00000000 00000000 00000000 00000000
# CHECK-NEXT: )

# CHECK:      Index: [[RELA:[0-9]+]]
# CHECK-NEXT: Name: .rela.data
# CHECK-NEXT: Type: SHT_RELA (0x4)
# CHECK-NEXT: Flags [ (0x40)
# CHECK-NEXT:   SHF_INFO_LINK (0x40)
# CHECK-NEXT: ]
# CHECK:      Size: 96
# CHECK:      Link: [[SYMTAB:[0-9]+]]
# CHECK-NEXT: Info: [[DATA]]
# CHECK-NEXT: AddressAlignment: 8
# CHECK-NEXT: EntrySize: 24

# CHECK:      Index: [[SYMTAB]]
# CHECK-NEXT: Name: .symtab
# CHECK-NEXT: Type: SHT_SYMTAB (0x2)

# CHECK:      Relocations [
# CHECK-NEXT:   Section ([[RELA]]) .rela.data {
# CHECK-NEXT:     Relocation {
# CHECK-NEXT:       Offset: 0x0
# CHECK-NEXT:       Type: R_MMIX_8 (1)
# CHECK-NEXT:       Symbol: external ([[EXTERNAL:[0-9]+]])
# CHECK-NEXT:       Addend: 0x1
# CHECK-NEXT:     }
# CHECK-NEXT:     Relocation {
# CHECK-NEXT:       Offset: 0x2
# CHECK-NEXT:       Type: R_MMIX_16 (2)
# CHECK-NEXT:       Symbol: external ([[EXTERNAL]])
# CHECK-NEXT:       Addend: 0xFFFFFFFFFFFFFFFE
# CHECK-NEXT:     }
# CHECK-NEXT:     Relocation {
# CHECK-NEXT:       Offset: 0x4
# CHECK-NEXT:       Type: R_MMIX_32 (4)
# CHECK-NEXT:       Symbol: external ([[EXTERNAL]])
# CHECK-NEXT:       Addend: 0x3
# CHECK-NEXT:     }
# CHECK-NEXT:     Relocation {
# CHECK-NEXT:       Offset: 0x8
# CHECK-NEXT:       Type: R_MMIX_64 (5)
# CHECK-NEXT:       Symbol: external ([[EXTERNAL]])
# CHECK-NEXT:       Addend: 0xFFFFFFFFFFFFFFFC
# CHECK-NEXT:     }
# CHECK-NEXT:   }
# CHECK-NEXT: ]
