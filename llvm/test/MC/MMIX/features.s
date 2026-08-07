# RUN: llvm-mc -triple=mmix -show-encoding < %s | FileCheck %s --check-prefix=ALL
# RUN: llvm-mc -triple=mmix -mattr=-system,+system,-cache,+cache,-virtual-memory,+virtual-memory \
# RUN:   -show-encoding < %s | FileCheck %s --check-prefix=ALL
# RUN: not llvm-mc -triple=mmix -mattr=-system < %s 2>&1 | FileCheck %s --check-prefix=NO-SYSTEM
# RUN: not llvm-mc -triple=mmix -mattr=-cache < %s 2>&1 | FileCheck %s --check-prefix=NO-CACHE
# RUN: not llvm-mc -triple=mmix -mattr=-virtual-memory < %s 2>&1 | FileCheck %s --check-prefix=NO-VM

TRAP 1, 2, 3
# ALL: TRAP 1, 2, 3{{.*}}[0x00,0x01,0x02,0x03]
# NO-SYSTEM: error: instruction requires: system

TRIP 4, 5, 6
# ALL: TRIP 4, 5, 6{{.*}}[0xff,0x04,0x05,0x06]
# NO-SYSTEM: error: instruction requires: system

RESUME 0
# ALL: RESUME 0{{.*}}[0xf9,0x00,0x00,0x00]
# NO-SYSTEM: error: instruction requires: system

SAVE r255
# ALL: SAVE r255{{.*}}[0xfa,0xff,0x00,0x00]
# NO-SYSTEM: error: instruction requires: system

UNSAVE r255
# ALL: UNSAVE r255{{.*}}[0xfb,0x00,0x00,0xff]
# NO-SYSTEM: error: instruction requires: system

LDUNC r1, r2, 4
# ALL: LDUNC r1, r2, 4{{.*}}[0x97,0x01,0x02,0x04]
# NO-CACHE: error: instruction requires: cache

LDVTS r1, r2, 4
# ALL: LDVTS r1, r2, 4{{.*}}[0x99,0x01,0x02,0x04]
# NO-VM: error: instruction requires: virtual-memory
