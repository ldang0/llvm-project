# RUN: not llvm-mc -triple=mmix -filetype=asm --output-asm-variant=1 %s \
# RUN:   -o %t 2>&1 | FileCheck %s
# RUN: test ! -s %t

ADD $1, $2, $3

# CHECK: error: invalid operand for instruction
