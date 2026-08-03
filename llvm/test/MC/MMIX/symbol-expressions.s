# RUN: llvm-mc -triple=mmix -filetype=asm %s -o - | FileCheck %s
# RUN: not llvm-mc -triple=mmix -filetype=obj %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=OBJ

# CHECK:      SETH r1, (symbol>>48)&65535
# CHECK:      INCMH r1, (symbol>>32)&65535
# CHECK:      INCML r1, (symbol>>16)&65535
# CHECK:      INCL r1, symbol&65535
# OBJ: error: MMIX expression operand is not relocatable

SETH r1, (symbol >> 48) & 65535
INCMH r1, (symbol >> 32) & 65535
INCML r1, (symbol >> 16) & 65535
INCL r1, symbol & 65535
