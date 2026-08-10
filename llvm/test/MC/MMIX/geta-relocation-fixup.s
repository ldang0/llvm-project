# RUN: split-file %s %t

# RUN: llvm-mc -triple=mmix -show-encoding %t/valid.s \
# RUN:   | FileCheck %s --check-prefix=ENCODING
# RUN: llvm-mc -triple=mmix -filetype=asm %t/valid.s \
# RUN:   | FileCheck %s --check-prefix=ASM
# RUN: llvm-mc -triple=mmix -filetype=null %t/valid.s

# RUN: not llvm-mc -triple=mmix -filetype=obj %t/expanding.s \
# RUN:   -o %t/expanding.o 2>&1 | FileCheck %s --check-prefix=OBJECT
# RUN: not test -e %t/expanding.o

# RUN: not llvm-mc -triple=mmix -show-encoding %t/invalid.s 2>&1 \
# RUN:   | FileCheck %s --check-prefix=INVALID

# RUN: not llvm-mc -triple=mmix -show-encoding %t/split-address.s 2>&1 \
# RUN:   | FileCheck %s --check-prefix=SPLIT \
# RUN:       --implicit-check-not=fixup_mmix_geta

# ENCODING:      GETA r3, %geta(external+7)
# ENCODING-SAME: encoding: [0xf4'A',0x03'A',0x00,0x00]
# ENCODING-NEXT: fixup A - offset: 0, value: external+7, kind: fixup_mmix_geta
# ENCODING:      GETA r6, %geta(external-9)
# ENCODING-SAME: encoding: [0xf4'A',0x06'A',0x00,0x00]
# ENCODING-NEXT: fixup A - offset: 0, value: external-9, kind: fixup_mmix_geta
# ENCODING:      GETA r4, local
# ENCODING-NEXT: fixup A - offset: 0, value: local, kind: fixup_mmix_branch_forward
# ENCODING:      GETAB r5, local
# ENCODING-NEXT: fixup A - offset: 0, value: local, kind: fixup_mmix_branch_backward

# ASM: GETA r3, %geta(external+7)

# OBJECT: expanding.s:1:25: error: expanding GETA relocation reservation is not implemented

# INVALID: invalid.s:1:10: error: expanding GETA requires one symbol plus an optional addend
# INVALID: invalid.s:2:10: error: expanding GETA requires one symbol plus an optional addend
# INVALID: invalid.s:3:10: error: expanding GETA requires one symbol plus an optional addend
# INVALID: invalid.s:4:1: error: '%geta' expression requires a GETA instruction
# INVALID: invalid.s:5:1: error: '%geta' expression requires a GETA instruction
# INVALID: invalid.s:6:11: error: expected '%geta' expression specifier

# SPLIT: split-address.s:1:1: error: unresolved MMIX symbolic instruction operand requires relocation support
# SPLIT: split-address.s:2:1: error: unresolved MMIX symbolic instruction operand requires relocation support
# SPLIT: split-address.s:3:1: error: unresolved MMIX symbolic instruction operand requires relocation support
# SPLIT: split-address.s:4:1: error: unresolved MMIX symbolic instruction operand requires relocation support

#--- valid.s
GETA r3, %geta(external + 7)
GETA r6, %geta(external - 9)
GETA r4, local
local:
GETAB r5, local

#--- expanding.s
GETA r3, %geta(external - 9)

#--- invalid.s
GETA r1, %geta(7)
GETA r1, %geta(first - second)
GETA r1, %geta(first + second)
GETAB r1, %geta(external)
BN r1, %geta(external)
GETA r1, %unknown(external)

#--- split-address.s
SETH r1, (symbol >> 48) & 65535
INCMH r1, (symbol >> 32) & 65535
INCML r1, (symbol >> 16) & 65535
INCL r1, symbol & 65535
