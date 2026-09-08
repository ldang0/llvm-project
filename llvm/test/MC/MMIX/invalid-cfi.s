# RUN: not llvm-mc -triple=mmix -filetype=obj %s -o /dev/null 2>&1 | FileCheck %s
# CHECK: error: this directive must appear between .cfi_startproc and .cfi_endproc directives
.cfi_def_cfa 30, 0
