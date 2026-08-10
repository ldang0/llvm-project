# RUN: split-file %s %t

# RUN: not llvm-mc -triple=mmix -filetype=obj %t/instruction.s \
# RUN:   -o %t/instruction.o 2>&1 | FileCheck %s --check-prefix=INSTRUCTION
# RUN: not test -e %t/instruction.o

# RUN: not llvm-mc -triple=mmix -filetype=obj %t/split-address.s \
# RUN:   -o %t/split-address.o 2>&1 | FileCheck %s --check-prefix=SPLIT
# RUN: not test -e %t/split-address.o

# RUN: llvm-mc -triple=mmix -filetype=obj %t/resolved-data.s \
# RUN:   -o %t/resolved-data.o
# RUN: llvm-readobj --sections --section-data --relocations %t/resolved-data.o \
# RUN:   | FileCheck %s --check-prefix=RESOLVED \
# RUN:       --implicit-check-not='Name: .rel' \
# RUN:       --implicit-check-not='Name: .rela'

# INSTRUCTION: instruction.s:1:20: error: MMIX stubbable call relocation is not implemented

# SPLIT: split-address.s:1:1: error: unresolved MMIX split-address expression is not supported; use GETA with '%geta(...)'
# SPLIT: split-address.s:2:1: error: unresolved MMIX split-address expression is not supported; use GETA with '%geta(...)'
# SPLIT: split-address.s:3:1: error: unresolved MMIX split-address expression is not supported; use GETA with '%geta(...)'
# SPLIT: split-address.s:4:1: error: unresolved MMIX split-address expression is not supported; use GETA with '%geta(...)'

# RESOLVED:      Name: .data
# RESOLVED:      Size: 16
# RESOLVED:      SectionData (
# RESOLVED-NEXT:   0000: 00010001 00000001 00000000 00000001
# RESOLVED-NEXT: )
# RESOLVED:      Relocations [
# RESOLVED-NEXT: ]

#--- instruction.s
PUSHJ r2, external - 8

#--- split-address.s
SETH r1, (symbol >> 48) & 65535
INCMH r1, (symbol >> 32) & 65535
INCML r1, (symbol >> 16) & 65535
INCL r1, symbol & 65535

#--- resolved-data.s
.data
start:
.byte 0
end:
.byte end-start
.short end-start
.long end-start
.quad end-start
