# RUN: llvm-mc -triple=mmix -show-encoding %s \
# RUN:   | FileCheck %s --implicit-check-not=fixup_mmix_geta \
# RUN:       --implicit-check-not=fixup_mmix_branch_forward \
# RUN:       --implicit-check-not=fixup_mmix_branch_backward

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

# Procedure calls use one call-specific fixup while retaining their opcode,
# register operand, and signed symbolic addend.
PUSHJ r5, external + 7
# CHECK:      PUSHJ r5, external+7{{.*}}encoding: [0xf2'A',0x05'A',0x00,0x00]
# CHECK-NEXT: fixup A - offset: 0, value: external+7, kind: fixup_mmix_call

PUSHJB r6, external - 9
# CHECK:      PUSHJB r6, external-9{{.*}}encoding: [0xf3'A',0x06'A',0x00,0x00]
# CHECK-NEXT: fixup A - offset: 0, value: external-9, kind: fixup_mmix_call

# Register-indirect calls do not create a direct-call fixup.
PUSHGO r7, r8, r9
# CHECK:      PUSHGO r7, r8, r9{{.*}}encoding: [0xbe,0x07,0x08,0x09]
# CHECK-NOT:  fixup
