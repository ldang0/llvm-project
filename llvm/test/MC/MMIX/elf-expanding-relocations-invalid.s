# RUN: split-file %s %t

# RUN: not llvm-mc -triple=mmix -filetype=obj %t/primary.s \
# RUN:   -o %t/primary.o 2>&1 | FileCheck %s --check-prefix=PRIMARY
# RUN: not test -e %t/primary.o

# RUN: not llvm-mc -triple=mmix -filetype=obj %t/intermediate.s \
# RUN:   -o %t/intermediate.o 2>&1 \
# RUN:   | FileCheck %s --check-prefix=INTERMEDIATE
# RUN: not test -e %t/intermediate.o

# RUN: llvm-mc -triple=mmix -filetype=obj %t/geta.s -o %t/geta.o
# RUN: llvm-readobj --sections --relocations --expand-relocs %t/geta.o \
# RUN:   | FileCheck %s --check-prefix=GETA

# PRIMARY: primary.s:1:27: error: R_MMIX_CBRANCH requires an assembler-owned 24-byte reservation
# PRIMARY: primary.s:4:32: error: R_MMIX_CBRANCH requires an assembler-owned 24-byte reservation
# PRIMARY: primary.s:6:25: error: R_MMIX_PUSHJ requires an assembler-owned 20-byte reservation
# PRIMARY: primary.s:9:28: error: R_MMIX_PUSHJ requires an assembler-owned 20-byte reservation
# PRIMARY: primary.s:11:23: error: R_MMIX_JMP requires an assembler-owned 20-byte reservation
# PRIMARY: primary.s:14:26: error: R_MMIX_JMP requires an assembler-owned 20-byte reservation

# INTERMEDIATE-COUNT-13: error: GNU MMIX intermediate relaxation relocations cannot be emitted directly

# The supported GETA producer owns exactly four instructions and emits only the
# primary R_MMIX_GETA record.
# GETA:          Name: .text
# GETA:          Size: 16
# GETA:          Relocations [
# GETA-NEXT:       Section ({{.*}}) .rela.text {
# GETA-NEXT:         Relocation {
# GETA-NEXT:           Offset: 0x0
# GETA-NEXT:           Type: R_MMIX_GETA (13)
# GETA-NEXT:           Symbol: external
# GETA-NEXT:           Addend: 0x0
# GETA-NEXT:         }
# GETA-NEXT:       }
# GETA-NEXT:     ]

# Test both an arbitrary raw relocation site and a lone instruction site for
# each primary expanding relocation.
#--- primary.s
.reloc ., R_MMIX_CBRANCH, external
branch:
BN r1, 0
.reloc branch, R_MMIX_CBRANCH, external

.reloc ., R_MMIX_PUSHJ, external
call:
PUSHJ r2, 1
.reloc call, R_MMIX_PUSHJ, external

.reloc ., R_MMIX_JMP, external
jump:
JMP 0
.reloc jump, R_MMIX_JMP, external

#--- intermediate.s
.reloc ., R_MMIX_CBRANCH_J, external
.reloc ., R_MMIX_CBRANCH_1, external
.reloc ., R_MMIX_CBRANCH_2, external
.reloc ., R_MMIX_CBRANCH_3, external
.reloc ., R_MMIX_PUSHJ_1, external
.reloc ., R_MMIX_PUSHJ_2, external
.reloc ., R_MMIX_PUSHJ_3, external
.reloc ., R_MMIX_JMP_1, external
.reloc ., R_MMIX_JMP_2, external
.reloc ., R_MMIX_JMP_3, external

# GETA intermediates are also checked by elf-geta-relocation-invalid.s.
.reloc ., R_MMIX_GETA_1, external
.reloc ., R_MMIX_GETA_2, external
.reloc ., R_MMIX_GETA_3, external

#--- geta.s
GETA r1, %geta(external)
