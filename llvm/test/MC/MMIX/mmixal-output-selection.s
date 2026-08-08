# RUN: llvm-mc -triple=mmix -filetype=asm --output-asm-variant=1 %s \
# RUN:   -o - | FileCheck %s --check-prefix=SOURCE --implicit-check-not=: \
# RUN:     --implicit-check-not='{{^[[:space:]]*\.}}'
# RUN: not llvm-mc -triple=mmix -filetype=obj --output-asm-variant=1 %s \
# RUN:   -o %t.o 2>&1 | FileCheck %s --check-prefix=OBJECT
# RUN: test ! -s %t.o
# RUN: not llvm-mc -triple=mmix -filetype=asm --output-asm-variant=42 %s \
# RUN:   -o %t.unknown 2>&1 | FileCheck %s --check-prefix=UNKNOWN
# RUN: test ! -s %t.unknown

# SOURCE: LOC #0000000000000100
# SOURCE-NEXT: ADD $1, $2, $3
# OBJECT: error: MMIXAL complete-source emission is not available
# UNKNOWN: error: unable to create instruction printer for target triple 'mmix' with assembly variant 42

ADD r1, r2, r3
