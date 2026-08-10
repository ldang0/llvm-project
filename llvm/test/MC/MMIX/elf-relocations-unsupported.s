# RUN: split-file %s %t

# RUN: not llvm-mc -triple=mmix -filetype=obj %t/instruction.s \
# RUN:   -o %t/instruction.o 2>&1 | FileCheck %s --check-prefix=INSTRUCTION
# RUN: not test -e %t/instruction.o

# RUN: not llvm-mc -triple=mmix -filetype=obj %t/inter-section.s \
# RUN:   -o %t/inter-section.o 2>&1 | FileCheck %s --check-prefix=INTER
# RUN: not test -e %t/inter-section.o

# RUN: not llvm-mc -triple=mmix -filetype=obj %t/weak.s \
# RUN:   -o %t/weak.o 2>&1 | FileCheck %s --check-prefix=WEAK
# RUN: not test -e %t/weak.o

# RUN: not llvm-mc -triple=mmix -filetype=obj %t/split-address.s \
# RUN:   -o %t/split-address.o 2>&1 | FileCheck %s --check-prefix=SPLIT
# RUN: not test -e %t/split-address.o

# RUN: llvm-mc -triple=mmix -filetype=obj %t/resolved-data.s \
# RUN:   -o %t/resolved-data.o
# RUN: llvm-readobj --sections --section-data --relocations %t/resolved-data.o \
# RUN:   | FileCheck %s --check-prefix=RESOLVED \
# RUN:       --implicit-check-not='Name: .rel' \
# RUN:       --implicit-check-not='Name: .rela'

# INSTRUCTION: instruction.s:1:8: error: MMIX 16-bit forward PC-relative instruction relocation is not implemented
# INSTRUCTION: instruction.s:2:5: error: MMIX 24-bit forward PC-relative instruction relocation is not implemented
# INSTRUCTION: instruction.s:3:11: error: MMIX 16-bit forward PC-relative instruction relocation is not implemented
# INSTRUCTION: instruction.s:4:10: error: MMIX 16-bit forward PC-relative instruction relocation is not implemented
# INSTRUCTION: instruction.s:5:9: error: MMIX 16-bit backward PC-relative instruction relocation is not implemented
# INSTRUCTION: instruction.s:6:6: error: MMIX 24-bit backward PC-relative instruction relocation is not implemented

# INTER: inter-section.s:2:8: error: MMIX 16-bit forward PC-relative instruction relocation is not implemented

# WEAK: weak.s:4:8: error: MMIX 16-bit forward PC-relative instruction relocation is not implemented

# SPLIT: split-address.s:1:1: error: unresolved MMIX symbolic instruction operand requires relocation support
# SPLIT: split-address.s:2:1: error: unresolved MMIX symbolic instruction operand requires relocation support
# SPLIT: split-address.s:3:1: error: unresolved MMIX symbolic instruction operand requires relocation support
# SPLIT: split-address.s:4:1: error: unresolved MMIX symbolic instruction operand requires relocation support

# RESOLVED:      Name: .data
# RESOLVED:      Size: 16
# RESOLVED:      SectionData (
# RESOLVED-NEXT:   0000: 00010001 00000001 00000000 00000001
# RESOLVED-NEXT: )
# RESOLVED:      Relocations [
# RESOLVED-NEXT: ]

#--- instruction.s
BN r1, external
JMP external
PUSHJ r2, external
GETA r3, external
BNB r4, external
JMPB external

#--- inter-section.s
.text
BN r1, data_target
.data
data_target:
.quad 0

#--- weak.s
.text
.weak weak_target
weak_target:
BN r1, weak_target

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
