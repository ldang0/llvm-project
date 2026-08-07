# RUN: not llvm-mc -triple=mmix -show-encoding %s -o /dev/null 2>&1 | FileCheck %s

ADD r256, r2, r3
# CHECK: error: invalid operand for instruction

ADD r1, r2, 256
# CHECK: error: invalid operand for instruction

ADD r1, r2, -1
# CHECK: error: invalid operand for instruction

ADD r1, 2, r3
# CHECK: error: invalid operand for instruction

NEG r1, -1, r2
# CHECK: error: invalid operand for instruction

NEG r1, 256, r2
# CHECK: error: invalid operand for instruction

NEG r1, 1, -1
# CHECK: error: invalid operand for instruction

GET r1, r1
# CHECK: error: invalid operand for instruction

GET r1, 0
# CHECK: error: invalid operand for instruction

PUT r1, r2
# CHECK: error: invalid operand for instruction

FSQRT r1, 5, r2
# CHECK: error: invalid operand for instruction

FSQRT r1, -1, r2
# CHECK: error: invalid operand for instruction

FLOT r1, 2, 256
# CHECK: error: invalid operand for instruction

SETH r1, 65536
# CHECK: error: invalid operand for instruction

SETH r1, -1
# CHECK: error: invalid operand for instruction

LDB r1, r2, -1
# CHECK: error: invalid operand for instruction

LDB r1, r2, 256
# CHECK: error: invalid operand for instruction

PRELD -1, r2, r3
# CHECK: error: invalid operand for instruction

PRELD 256, r2, r3
# CHECK: error: invalid operand for instruction

PRELD 1, r2, -1
# CHECK: error: invalid operand for instruction

POP -1, 0
# CHECK: error: invalid operand for instruction

POP r1, 0
# CHECK: error: invalid operand for instruction

POP 0, 65536
# CHECK: error: invalid operand for instruction

RESUME 2
# CHECK: error: invalid operand for instruction

RESUME -1
# CHECK: error: invalid operand for instruction

RESUME r1
# CHECK: error: invalid operand for instruction

TRAP 256, 1, 2
# CHECK: error: invalid operand for instruction

TRIP 1, -1, 2
# CHECK: error: invalid operand for instruction

SAVE 255
# CHECK: error: invalid operand for instruction

UNSAVE rA
# CHECK: error: invalid operand for instruction

SYNC 8
# CHECK: error: invalid operand for instruction

SYNC -1
# CHECK: error: invalid operand for instruction

SYNC r1
# CHECK: error: invalid operand for instruction

PUT rA, -1
# CHECK: error: invalid operand for instruction

PUT rA, 256
# CHECK: error: invalid operand for instruction

SET r1, 1
# CHECK: error: invalid instruction

LDA r1, 1
# CHECK: error: invalid instruction

ADDI r1, r2, 1
# CHECK: error: invalid instruction

TRAP 1
# CHECK: error: invalid operand for instruction

SWYM 1, 2
# CHECK: error: invalid operand for instruction

BN r1, -1
# CHECK: error: MMIX PC-relative operand is out of range

BN r1, 65536
# CHECK: error: MMIX PC-relative operand is out of range

BNB r1, 0
# CHECK: error: MMIX PC-relative operand is out of range

BNB r1, -65537
# CHECK: error: MMIX PC-relative operand is out of range

JMP -1
# CHECK: error: MMIX PC-relative operand is out of range

JMP 16777216
# CHECK: error: MMIX PC-relative operand is out of range

JMPB 0
# CHECK: error: MMIX PC-relative operand is out of range

JMPB -16777217
# CHECK: error: MMIX PC-relative operand is out of range
