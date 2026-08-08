# RUN: llvm-mc -triple=mmix -filetype=asm --output-asm-variant=1 %s -o %t
# RUN: test ! -s %t
