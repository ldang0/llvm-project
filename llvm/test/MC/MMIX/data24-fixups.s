# RUN: split-file %s %t

# RUN: llvm-mc -triple=mmix -filetype=asm %t/syntax.s \
# RUN:   | FileCheck %s --check-prefix=ASM
# RUN: llvm-mc -triple=mmix -filetype=null %t/syntax.s

# RUN: llvm-mc -triple=mmix -filetype=obj %t/resolved.s \
# RUN:   -o %t/resolved.o
# RUN: llvm-readobj --sections --section-data --relocations %t/resolved.o \
# RUN:   | FileCheck %s --check-prefix=RESOLVED

# RUN: llvm-mc -triple=mmix -filetype=obj %t/unresolved.s \
# RUN:   -o %t/unresolved.o
# RUN: llvm-readobj --sections --section-data --symbols --relocations \
# RUN:   --expand-relocs %t/unresolved.o \
# RUN:   | FileCheck %s --check-prefix=UNRESOLVED

# RUN: llvm-mc -triple=mmix -filetype=obj %t/ordinary-long.s \
# RUN:   -o %t/ordinary-long.o
# RUN: llvm-readobj --relocations %t/ordinary-long.o \
# RUN:   | FileCheck %s --check-prefix=ORDINARY

# ASM:      .mmix_24 165, external+7
# ASM-NEXT: .mmix_pc_24 90, external-9
# ASM-NEXT: .4byte external

# RESOLVED:      Name: .data24
# RESOLVED:      Size: 18
# RESOLVED:      SectionData (
# RESOLVED-NEXT:   0000: A5123456 C3FFFFFE 5A000004 11227EFF
# RESOLVED-NEXT:   0010: FFFF
# RESOLVED-NEXT: )
# RESOLVED:      Relocations [
# RESOLVED-NEXT: ]

# UNRESOLVED:      Name: .data24
# UNRESOLVED:      Type: SHT_PROGBITS
# UNRESOLVED:      Size: 18
# UNRESOLVED:      AddressAlignment: 1
# UNRESOLVED:      EntrySize: 0
# UNRESOLVED:      SectionData (
# UNRESOLVED-NEXT:   0000: A5000000 CC5A0000 00C30000 00DD7E00
# UNRESOLVED-NEXT:   0010: 0000
# UNRESOLVED-NEXT: )
# UNRESOLVED:      Name: .rela.data24
# UNRESOLVED:      Type: SHT_RELA
# UNRESOLVED:      Size: 96
# UNRESOLVED:      Link: 5
# UNRESOLVED:      Info: 3
# UNRESOLVED:      AddressAlignment: 8
# UNRESOLVED:      EntrySize: 24
# UNRESOLVED:      Relocations [
# UNRESOLVED-NEXT:   Section {{.*}} .rela.data24 {
# UNRESOLVED-NEXT:     Relocation {
# UNRESOLVED-NEXT:       Offset: 0x0
# UNRESOLVED-NEXT:       Type: R_MMIX_24 (3)
# UNRESOLVED-NEXT:       Symbol: external
# UNRESOLVED-NEXT:       Addend: 0x7
# UNRESOLVED-NEXT:     }
# UNRESOLVED-NEXT:     Relocation {
# UNRESOLVED-NEXT:       Offset: 0x5
# UNRESOLVED-NEXT:       Type: R_MMIX_24 (3)
# UNRESOLVED-NEXT:       Symbol: weak_external
# UNRESOLVED-NEXT:       Addend: 0xFFFFFFFFFFFFFFF7
# UNRESOLVED-NEXT:     }
# UNRESOLVED-NEXT:     Relocation {
# UNRESOLVED-NEXT:       Offset: 0x9
# UNRESOLVED-NEXT:       Type: R_MMIX_PC_24 (8)
# UNRESOLVED-NEXT:       Symbol: external
# UNRESOLVED-NEXT:       Addend: 0xB
# UNRESOLVED-NEXT:     }
# UNRESOLVED-NEXT:     Relocation {
# UNRESOLVED-NEXT:       Offset: 0xE
# UNRESOLVED-NEXT:       Type: R_MMIX_PC_24 (8)
# UNRESOLVED-NEXT:       Symbol: weak_external
# UNRESOLVED-NEXT:       Addend: 0xFFFFFFFFFFFFFFF3
# UNRESOLVED-NEXT:     }
# UNRESOLVED-NEXT:   }
# UNRESOLVED-NEXT: ]
# UNRESOLVED:      Name: external
# UNRESOLVED:      Binding: Global
# UNRESOLVED:      Section: Undefined
# UNRESOLVED:      Name: weak_external
# UNRESOLVED:      Binding: Weak
# UNRESOLVED:      Section: Undefined

# ORDINARY:      Relocations [
# ORDINARY-NEXT:   Section {{.*}} .rela.data {
# ORDINARY-NEXT:     0x0 R_MMIX_32 external 0x0
# ORDINARY-NEXT:     0x4 R_MMIX_PC_32 external 0x0
# ORDINARY-NEXT:   }
# ORDINARY-NEXT: ]

#--- syntax.s
.data
.mmix_24 165, external + 7
.mmix_pc_24 90, external - 9
.long external

#--- resolved.s
.section .data24,"aw",@progbits
.mmix_24 0xa5, 0x123456
.mmix_24 0xc3, -2
.mmix_pc_24 0x5a, .Lforward
.Lforward:
.byte 0x11
.Lbackward:
.byte 0x22
.mmix_pc_24 0x7e, .Lbackward

#--- unresolved.s
.global external
.weak weak_external
.section .data24,"aw",@progbits
.mmix_24 0xa5, external + 7
.byte 0xcc
.mmix_24 0x5a, weak_external - 9
.mmix_pc_24 0xc3, external + 11
.byte 0xdd
.mmix_pc_24 0x7e, weak_external - 13

#--- ordinary-long.s
.data
.long external
.Lplace:
.long external - .Lplace
