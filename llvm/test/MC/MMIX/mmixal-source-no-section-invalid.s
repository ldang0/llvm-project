# RUN: not llvm-mc -triple=mmix -n -filetype=asm --output-asm-variant=1 %s \
# RUN:   -o %t 2>&1 | FileCheck %s
# RUN: test ! -s %t

ADD r1, r2, r3

# CHECK: error: expected section directive before assembly directive
