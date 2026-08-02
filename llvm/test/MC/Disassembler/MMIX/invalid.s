# RUN: echo '0xfe 0x01 0x00 0x20' | llvm-mc -triple=mmix -disassemble 2>&1 | FileCheck %s --check-prefix=INVALID
# RUN: echo '0xfe 0x01 0x01 0x00' | llvm-mc -triple=mmix -disassemble 2>&1 | FileCheck %s --check-prefix=INVALID
# RUN: echo '0xf6 0x15 0x01 0x01' | llvm-mc -triple=mmix -disassemble 2>&1 | FileCheck %s --check-prefix=INVALID
# RUN: echo '0x05 0x01 0x05 0x02' | llvm-mc -triple=mmix -disassemble 2>&1 | FileCheck %s --check-prefix=INVALID
# RUN: echo '0xf9 0x01 0x00 0x00' | llvm-mc -triple=mmix -disassemble 2>&1 | FileCheck %s --check-prefix=INVALID
# RUN: echo '0xf9 0x00 0x00 0x02' | llvm-mc -triple=mmix -disassemble 2>&1 | FileCheck %s --check-prefix=INVALID
# RUN: echo '0xfa 0x01 0x00 0x01' | llvm-mc -triple=mmix -disassemble 2>&1 | FileCheck %s --check-prefix=INVALID
# RUN: echo '0xfb 0x01 0x00 0x01' | llvm-mc -triple=mmix -disassemble 2>&1 | FileCheck %s --check-prefix=INVALID
# RUN: echo '0xfc 0x00 0x00 0x08' | llvm-mc -triple=mmix -disassemble 2>&1 | FileCheck %s --check-prefix=INVALID

# INVALID: warning: invalid instruction encoding
