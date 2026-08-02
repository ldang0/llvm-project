# RUN: llvm-mc -triple=mmix -filetype=obj %s -o /dev/null
# RUN: llvm-mc -triple=mmix -filetype=obj %s -o %t
# RUN: llvm-objdump --triple=mmix --no-print-imm-hex -d \
# RUN:   --start-address=0 --stop-address=4 %t | FileCheck %s --check-prefix=FWD
# RUN: llvm-objdump --triple=mmix --no-print-imm-hex -d \
# RUN:   --start-address=524284 --stop-address=524288 %t | FileCheck %s --check-prefix=BWD

# The forward target is 65,535 instruction words from the branch.
BN r1, forward_limit
# FWD: BN r1, 65535
.space 262136
forward_limit:

# The backward target is 65,536 instruction words from the branch.
backward_limit:
.space 262144
BNB r1, backward_limit
# BWD: BNB r1, -65536
