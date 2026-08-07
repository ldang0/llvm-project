# RUN: not llvm-mc -triple=mmix -filetype=obj %s -o /dev/null 2>&1 | FileCheck %s

branch_misaligned:
BN r1, branch_misaligned_target
.set branch_misaligned_target, branch_misaligned + 2
# CHECK: error: MMIX PC-relative fixup is not instruction aligned

branch_forward_direction:
BN r1, branch_before
.set branch_before, branch_forward_direction - 4
# CHECK: error: MMIX PC-relative fixup is out of range

branch_backward_direction:
BNB r1, branch_after
.set branch_after, branch_backward_direction + 4
# CHECK: error: MMIX PC-relative fixup is out of range

branch_forward_range:
BN r1, branch_forward_out_of_range
.set branch_forward_out_of_range, branch_forward_range + 262144
# CHECK: error: MMIX PC-relative fixup is out of range

branch_backward_range:
BNB r1, branch_backward_out_of_range
.set branch_backward_out_of_range, branch_backward_range - 262148
# CHECK: error: MMIX PC-relative fixup is out of range

jump_misaligned:
JMP jump_misaligned_target
.set jump_misaligned_target, jump_misaligned + 2
# CHECK: error: MMIX PC-relative fixup is not instruction aligned

jump_forward_direction:
JMP jump_before
.set jump_before, jump_forward_direction - 4
# CHECK: error: MMIX PC-relative fixup is out of range

jump_backward_direction:
JMPB jump_after
.set jump_after, jump_backward_direction + 4
# CHECK: error: MMIX PC-relative fixup is out of range

jump_forward_range:
JMP jump_forward_out_of_range
.set jump_forward_out_of_range, jump_forward_range + 67108864
# CHECK: error: MMIX PC-relative fixup is out of range

jump_backward_range:
JMPB jump_backward_out_of_range
.set jump_backward_out_of_range, jump_backward_range - 67108868
# CHECK: error: MMIX PC-relative fixup is out of range
