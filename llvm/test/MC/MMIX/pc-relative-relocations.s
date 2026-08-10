# RUN: echo 'BN r1, external' | not llvm-mc -triple=mmix -filetype=obj \
# RUN:   -o /dev/null 2>&1 | FileCheck %s --check-prefix=BRANCH
# RUN: echo 'JMP external' | not llvm-mc -triple=mmix -filetype=obj \
# RUN:   -o /dev/null 2>&1 | FileCheck %s --check-prefix=JUMP

# BRANCH: error: MMIX 19-bit direction-neutral PC-relative instruction relocation is not implemented
# JUMP: error: MMIX 27-bit direction-neutral PC-relative instruction relocation is not implemented
