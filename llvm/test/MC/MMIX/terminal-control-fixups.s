# RUN: llvm-mc -triple=mmix -show-encoding %s \
# RUN:   | FileCheck %s --implicit-check-not=fixup_mmix_geta

BN r1, target
# CHECK:      BN r1, target
# CHECK-NEXT: fixup A - offset: 0, value: target, kind: fixup_mmix_addr19

BNB r2, target
# CHECK:      BNB r2, target
# CHECK-NEXT: fixup A - offset: 0, value: target, kind: fixup_mmix_addr19

GETA r3, target
# CHECK:      GETA r3, target
# CHECK-NEXT: fixup A - offset: 0, value: target, kind: fixup_mmix_addr19

GETAB r4, target
# CHECK:      GETAB r4, target
# CHECK-NEXT: fixup A - offset: 0, value: target, kind: fixup_mmix_addr19

JMP target
# CHECK:      JMP target
# CHECK-NEXT: fixup A - offset: 0, value: target, kind: fixup_mmix_addr27

JMPB target
# CHECK:      JMPB target
# CHECK-NEXT: fixup A - offset: 0, value: target, kind: fixup_mmix_addr27

# Procedure calls retain their directional local fixups until the dedicated
# stubbable-call fixup is introduced.
PUSHJ r5, target
# CHECK:      PUSHJ r5, target
# CHECK-NEXT: fixup A - offset: 0, value: target, kind: fixup_mmix_branch_forward

PUSHJB r6, target
# CHECK:      PUSHJB r6, target
# CHECK-NEXT: fixup A - offset: 0, value: target, kind: fixup_mmix_branch_backward
