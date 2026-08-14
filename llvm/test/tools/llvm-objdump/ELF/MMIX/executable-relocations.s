## Disassemble terminal control transfers after lld has consumed their
## relocations, including both forward and backward directions.

# RUN: rm -rf %t && split-file %s %t
# RUN: llvm-mc -triple=mmix -filetype=obj %t/backward.s -o %t/backward.o
# RUN: llvm-mc -triple=mmix -filetype=obj %t/use.s -o %t/use.o
# RUN: llvm-mc -triple=mmix -filetype=obj %t/forward.s -o %t/forward.o
# RUN: ld.lld -e inspection_entry -Ttext=0x40000 %t/backward.o %t/use.o \
# RUN:   %t/forward.o -o %t/executable
# RUN: llvm-objdump --no-print-imm-hex -d %t/executable \
# RUN:   | FileCheck %s --check-prefix=DIS
# RUN: llvm-objdump -r %t/executable \
# RUN:   | FileCheck %s --check-prefix=RELOCS --implicit-check-not=R_MMIX_

# DIS-LABEL: <inspection_entry>:
# DIS-NEXT:  {{.*}} BN r1, 4
# DIS-NEXT:  {{.*}} BNB r2, -2
# DIS-NEXT:  {{.*}} JMP 2
# DIS-NEXT:  {{.*}} JMPB -4
# DIS-LABEL: <forward_target>:
# RELOCS: file format elf64-mmix

#--- backward.s
.text
.global backward_target
backward_target:
SWYM 0, 0, 0

#--- use.s
.text
.global inspection_entry
.type inspection_entry,@function
inspection_entry:
BN r1, forward_target
BN r2, backward_target
JMP forward_target
JMP backward_target
.size inspection_entry, .-inspection_entry
.global forward_target
.global backward_target

#--- forward.s
.text
.global forward_target
forward_target:
SWYM 0, 0, 0
