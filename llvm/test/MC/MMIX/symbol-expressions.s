# RUN: llvm-mc -triple=mmix -filetype=asm %s -o - | FileCheck %s
# RUN: not llvm-mc -triple=mmix -filetype=obj %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=OBJ

# CHECK:      SETH r1, (symbol>>48)&65535
# CHECK:      INCMH r1, (symbol>>32)&65535
# CHECK:      INCML r1, (symbol>>16)&65535
# CHECK:      INCL r1, symbol&65535
# OBJ-COUNT-4: error: unresolved MMIX split-address expression is not supported; use GETA with '%geta(...)'

SETH r1, (symbol >> 48) & 65535
INCMH r1, (symbol >> 32) & 65535
INCML r1, (symbol >> 16) & 65535
INCL r1, symbol & 65535
