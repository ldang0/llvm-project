# RUN: llvm-mc -triple=mmix -disassemble < %s 2>&1 | FileCheck %s

# An instruction is fixed-width and truncated input is invalid.
0x20 0x01 0x02
# CHECK: warning: invalid instruction encoding
