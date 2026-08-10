# RUN: split-file %s %t

# RUN: not llvm-mc -triple=mmix -filetype=obj %t/reservation.s \
# RUN:   -o %t/reservation.o 2>&1 | FileCheck %s --check-prefix=RESERVATION
# RUN: not test -e %t/reservation.o

# RUN: not llvm-mc -triple=mmix -filetype=obj %t/misaligned.s \
# RUN:   -o %t/misaligned.o 2>&1 | FileCheck %s --check-prefix=MISALIGNED
# RUN: not test -e %t/misaligned.o

# RUN: not llvm-mc -triple=mmix -filetype=obj %t/expression.s \
# RUN:   -o %t/expression.o 2>&1 | FileCheck %s --check-prefix=EXPRESSION
# RUN: not test -e %t/expression.o

# RUN: not llvm-mc -triple=mmix -filetype=obj %t/intermediate.s \
# RUN:   -o %t/intermediate.o 2>&1 | FileCheck %s --check-prefix=INTERMEDIATE
# RUN: not test -e %t/intermediate.o

# RESERVATION: reservation.s:1:24: error: R_MMIX_GETA requires an assembler-owned 16-byte reservation
# RESERVATION: reservation.s:4:36: error: R_MMIX_GETA requires an assembler-owned 16-byte reservation
# RESERVATION: reservation.s:11:38: error: R_MMIX_GETA requires an assembler-owned 16-byte reservation
# RESERVATION: reservation.s:18:36: error: R_MMIX_GETA requires an assembler-owned 16-byte reservation

# MISALIGNED: misaligned.s:1:23: error: MMIX GETA target is not instruction aligned

# EXPRESSION: expression.s:2:10: error: expanding GETA requires one symbol plus an optional addend
# EXPRESSION: expression.s:3:10: error: expanding GETA requires one symbol plus an optional addend
# EXPRESSION: expression.s:4:10: error: expanding GETA requires one symbol plus an optional addend
# EXPRESSION: expression.s:5:10: error: expanding GETA requires one symbol plus an optional addend
# EXPRESSION: expression.s:6:10: error: expanding GETA requires one symbol plus an optional addend

# INTERMEDIATE: intermediate.s:1:11: error: unknown relocation name
# INTERMEDIATE: intermediate.s:2:11: error: unknown relocation name
# INTERMEDIATE: intermediate.s:3:11: error: unknown relocation name

#--- reservation.s
.reloc ., R_MMIX_GETA, external

truncated_one:
.reloc truncated_one, R_MMIX_GETA, external
GETA r1, 0

truncated_three:
GETA r2, 0
SWYM 0, 0, 0
SWYM 0, 0, 0
.reloc truncated_three, R_MMIX_GETA, external

misplaced:
GETA r3, 0
SWYM 0, 0, 0
SWYM 0, 0, 0
SWYM 0, 0, 0
.reloc misplaced + 4, R_MMIX_GETA, external

#--- misaligned.s
GETA r1, %geta(target + 1)
target:
SWYM 0, 0, 0

#--- expression.s
GETA r1, %geta(valid_external)
GETA r2, %geta(first - second)
GETA r3, %geta(first + second)
GETA r4, %geta(first << 2)
GETA r5, %geta((first >> 48) & 65535)
GETA r6, %geta(first + (second - second))

#--- intermediate.s
.reloc ., R_MMIX_GETA_1, external
.reloc ., R_MMIX_GETA_2, external
.reloc ., R_MMIX_GETA_3, external
