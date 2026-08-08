# RUN: not llvm-mc -triple=mmix -filetype=asm --output-asm-variant=1 %s \
# RUN:   -o %t 2>&1 | FileCheck %s
# RUN: test ! -s %t

entry:
  ADD r1, r2, r3
.globl entry

# CHECK: error: MMIXAL does not support symbol linkage or visibility events
# CHECK-NOT: ADD $1, $2, $3
