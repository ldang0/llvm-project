# RUN: llvm-mc -triple=mmix -disassemble -show-encoding < %s | FileCheck %s --check-prefix=ALL
# RUN: llvm-mc -triple=mmix -mattr=-system -disassemble < %s 2>&1 | FileCheck %s --check-prefix=NO-SYSTEM
# RUN: llvm-mc -triple=mmix -mattr=-cache -disassemble < %s 2>&1 | FileCheck %s --check-prefix=NO-CACHE
# RUN: llvm-mc -triple=mmix -mattr=-virtual-memory -disassemble < %s 2>&1 | FileCheck %s --check-prefix=NO-VM

0x00 0x01 0x02 0x03
# ALL: TRAP 1, 2, 3{{.*}}[0x00,0x01,0x02,0x03]
# NO-SYSTEM: warning: invalid instruction encoding

0x97 0x01 0x02 0x04
# ALL: LDUNC r1, r2, 4{{.*}}[0x97,0x01,0x02,0x04]
# NO-CACHE: warning: invalid instruction encoding

0x99 0x01 0x02 0x04
# ALL: LDVTS r1, r2, 4{{.*}}[0x99,0x01,0x02,0x04]
# NO-VM: warning: invalid instruction encoding
