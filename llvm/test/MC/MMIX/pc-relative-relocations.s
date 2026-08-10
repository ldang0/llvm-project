# RUN: echo 'BN r1, external' | llvm-mc -triple=mmix -filetype=obj \
# RUN:   -o %t.branch
# RUN: llvm-readobj --relocations --expand-relocs %t.branch \
# RUN:   | FileCheck %s --check-prefix=BRANCH
# RUN: echo 'JMP external' | llvm-mc -triple=mmix -filetype=obj \
# RUN:   -o %t.jump
# RUN: llvm-readobj --relocations --expand-relocs %t.jump \
# RUN:   | FileCheck %s --check-prefix=JUMP

# BRANCH:      Type: R_MMIX_ADDR19 (30)
# BRANCH-NEXT: Symbol: external
# BRANCH-NEXT: Addend: 0x0
# JUMP:        Type: R_MMIX_ADDR27 (31)
# JUMP-NEXT:   Symbol: external
# JUMP-NEXT:   Addend: 0x0
