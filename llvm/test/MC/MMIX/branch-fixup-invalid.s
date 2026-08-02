# RUN: not llvm-mc -triple=mmix -filetype=obj %s -o /dev/null 2>&1 | FileCheck %s

BN r1, misaligned
.byte 0
misaligned:
# CHECK: error: MMIX PC-relative fixup is not instruction aligned

BN r1, out_of_range
.space 262144
out_of_range:
# CHECK: error: MMIX PC-relative fixup is out of range
