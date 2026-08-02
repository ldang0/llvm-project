# RUN: llvm-mc -triple=mmix -filetype=obj %s -o %t
# RUN: llvm-objdump --triple=mmix --no-print-imm-hex -d %t | FileCheck %s

ADD r1, r2, r3
ADD r4, r5, 255
FIXU r6, 4, r7
SETH r8, 4660
CSWAP r9, r10, 11
BNB r12, -2
JMPB -2
PUSHJB r13, -2
GETAB r14, -2
PUT rA, r15
RESUME 1
SYNC 7
GET r16, rA

# CHECK: ADD r1, r2, r3
# CHECK: ADD r4, r5, 255
# CHECK: FIXU r6, 4, r7
# CHECK: SETH r8, 4660
# CHECK: CSWAP r9, r10, 11
# CHECK: BNB r12, -2
# CHECK: JMPB -2
# CHECK: PUSHJB r13, -2
# CHECK: GETAB r14, -2
# CHECK: PUT rA, r15
# CHECK: RESUME 1
# CHECK: SYNC 7
# CHECK: GET r16, rA
