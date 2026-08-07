# RUN: not llvm-mc -triple=mmix -show-encoding %s -o /dev/null 2>&1 | FileCheck %s

ADD r256, r2, r3
# CHECK: error: invalid operand for instruction

ADD r1, r2, 256
# CHECK: error: invalid operand for instruction

GET r1, r1
# CHECK: error: invalid operand for instruction

FSQRT r1, 5, r2
# CHECK: error: invalid operand for instruction

SETH r1, 65536
# CHECK: error: invalid operand for instruction

RESUME 2
# CHECK: error: invalid operand for instruction

TRAP 256, 1, 2
# CHECK: error: invalid operand for instruction

TRIP 1, -1, 2
# CHECK: error: invalid operand for instruction

SYNC 8
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
