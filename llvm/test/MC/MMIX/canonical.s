# RUN: llvm-mc -triple=mmix -show-encoding %s | FileCheck %s
# RUN: llvm-mc -triple=mmix -filetype=asm %s -o %t.default
# RUN: llvm-mc -triple=mmix -filetype=asm --output-asm-variant=0 %s \
# RUN:   -o %t.explicit
# RUN: diff -u %t.default %t.explicit

ADD r1, r2, r3
# CHECK: ADD r1, r2, r3{{.*}}[0x20,0x01,0x02,0x03]

ADD r1, r2, 255
# CHECK: ADD r1, r2, 255{{.*}}[0x21,0x01,0x02,0xff]

LDB r1, r2, 4
# CHECK: LDB r1, r2, 4{{.*}}[0x81,0x01,0x02,0x04]

BN r1, target
# CHECK: BN r1, target
# CHECK: fixup_mmix_addr19

target:
JMP target
# CHECK: JMP target
# CHECK: fixup_mmix_addr27

GET r1, rA
# CHECK: GET r1, rA{{.*}}[0xfe,0x01,0x00,0x15]

PUT rA, 1
# CHECK: PUT rA, 1{{.*}}[0xf7,0x15,0x00,0x01]

SWYM 1, r2, 3
# CHECK: SWYM 1, r2, 3{{.*}}[0xfd,0x01,0x02,0x03]
