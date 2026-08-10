# RUN: split-file %s %t

# RUN: llvm-mc -triple=mmix -filetype=asm %t/syntax.s \
# RUN:   | FileCheck %s --check-prefix=ASM
# RUN: llvm-mc -triple=mmix -filetype=null %t/syntax.s

# RUN: llvm-mc -triple=mmix -filetype=obj %t/resolved.s \
# RUN:   -o %t/resolved.o
# RUN: llvm-readobj --sections --section-data --relocations %t/resolved.o \
# RUN:   | FileCheck %s --check-prefix=RESOLVED

# RUN: not llvm-mc -triple=mmix -filetype=obj %t/unresolved.s \
# RUN:   -o %t/unresolved.o 2>&1 | FileCheck %s --check-prefix=UNRESOLVED
# RUN: not test -e %t/unresolved.o

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

# UNRESOLVED: unresolved.s:2:16: error: MMIX 24-in-32 absolute data relocation is not implemented
# UNRESOLVED: unresolved.s:3:19: error: MMIX 24-in-32 PC-relative data relocation is not implemented

# ORDINARY:      Relocations [
# ORDINARY-NEXT:   Section {{.*}} .rela.data {
# ORDINARY-NEXT:     0x0 R_MMIX_32 external 0x0
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
.data
.mmix_24 0xa5, external + 7
.mmix_pc_24 0x5a, external - 9

#--- ordinary-long.s
.data
.long external
