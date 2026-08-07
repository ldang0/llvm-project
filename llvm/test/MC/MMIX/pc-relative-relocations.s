# RUN: echo 'BN r1, external' | not llvm-mc -triple=mmix -filetype=obj \
# RUN:   -o /dev/null 2>&1 | FileCheck %s
# RUN: echo 'JMP external' | not llvm-mc -triple=mmix -filetype=obj \
# RUN:   -o /dev/null 2>&1 | FileCheck %s

# CHECK: error: unresolved MMIX PC-relative fixup requires relocation support
