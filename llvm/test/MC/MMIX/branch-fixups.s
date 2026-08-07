# RUN: llvm-mc -triple=mmix -filetype=obj %s -o %t
# RUN: llvm-objdump --triple=mmix --no-print-imm-hex -d %t | FileCheck %s

# The forward target is 65,535 instruction words from the branch.
branch_forward:
BN r1, branch_forward_limit
.set branch_forward_limit, branch_forward + 262140
# CHECK: BN r1, 65535

# The backward target is 65,536 instruction words from the branch.
branch_backward:
BNB r1, branch_backward_limit
.set branch_backward_limit, branch_backward - 262144
# CHECK: BNB r1, -65536

# GETA uses the same architectural displacement boundaries.
address_forward:
GETA r2, address_forward_limit
.set address_forward_limit, address_forward + 262140
# CHECK: GETA r2, 65535

address_backward:
GETAB r2, address_backward_limit
.set address_backward_limit, address_backward - 262144
# CHECK: GETAB r2, -65536

# The jump boundaries are 16,777,215 words forward and 16,777,216 backward.
jump_forward:
JMP jump_forward_limit
.set jump_forward_limit, jump_forward + 67108860
# CHECK: JMP 16777215

jump_backward:
JMPB jump_backward_limit
.set jump_backward_limit, jump_backward - 67108864
# CHECK: JMPB -16777216
