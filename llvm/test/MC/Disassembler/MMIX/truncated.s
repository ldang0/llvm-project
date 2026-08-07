# RUN: echo '0x20' | llvm-mc -triple=mmix -disassemble 2>&1 | FileCheck %s
# RUN: echo '0x20 0x01' | llvm-mc -triple=mmix -disassemble 2>&1 | FileCheck %s
# RUN: echo '0x20 0x01 0x02' | llvm-mc -triple=mmix -disassemble 2>&1 | FileCheck %s

# An instruction is fixed-width and every nonempty truncated input is invalid.
# CHECK: warning: invalid instruction encoding
