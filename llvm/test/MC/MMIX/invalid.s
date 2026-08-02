# RUN: not llvm-mc -triple=mmix -show-encoding %s -o /dev/null 2>&1 | FileCheck %s

ADD r256, r2, r3
# CHECK: error: invalid operand for instruction

ADD r1, r2, 256
# CHECK: error: invalid operand for instruction

GET r1, r1
# CHECK: error: invalid operand for instruction

FSQRT r1, 4, r2
# CHECK: error: invalid operand for instruction
