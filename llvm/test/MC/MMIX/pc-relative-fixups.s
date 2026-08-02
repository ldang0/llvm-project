# RUN: llvm-mc -triple=mmix -filetype=obj %s -o %t
# RUN: llvm-objdump --triple=mmix --no-print-imm-hex -d %t | FileCheck %s

start:
PUSHJ r1, target
GETA r2, target
target:
PUSHJB r3, start
GETAB r4, start
JMP end
JMPB start
end:

# CHECK: PUSHJ r1, 2
# CHECK: GETA r2, 1
# CHECK: PUSHJB r3, -2
# CHECK: GETAB r4, -3
# CHECK: JMP 2
# CHECK: JMPB -5
