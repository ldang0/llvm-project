# RUN: not llvm-mc -triple=mmix -filetype=asm --output-asm-variant=1 %s \
# RUN:   -o %t.s 2>&1 | FileCheck %s --check-prefix=SOURCE
# RUN: test ! -s %t.s
# RUN: not llvm-mc -triple=mmix -filetype=obj --output-asm-variant=1 %s \
# RUN:   -o %t.o 2>&1 | FileCheck %s --check-prefix=OBJECT
# RUN: test ! -s %t.o

# SOURCE: error: MMIXAL complete-source emission is not available
# SOURCE-NOT: ADD $1
# OBJECT: error: MMIXAL complete-source emission is not available

ADD r1, r2, r3
