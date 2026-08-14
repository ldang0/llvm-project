# REQUIRES: mmix

# RUN: rm -rf %t && split-file %s %t
# RUN: llvm-mc -triple=mmix -filetype=obj %t/use.s -o %t/use.o
# RUN: llvm-mc -triple=mmix -filetype=obj %t/first.s -o %t/first.o
# RUN: llvm-mc -triple=mmix -filetype=obj %t/second.s -o %t/second.o
# RUN: ld.lld -e _start %t/use.o %t/first.o %t/second.o -o %t/out
# RUN: llvm-readobj --section-groups --sections --symbols --relocations %t/out \
# RUN:   | FileCheck %s --check-prefix=OBJ \
# RUN:     --implicit-check-not=losing_marker
# RUN: llvm-objdump --no-print-imm-hex -d %t/out \
# RUN:   | FileCheck %s --check-prefix=DIS

# OBJ:      Relocations [
# OBJ-NEXT: ]
# OBJ:      Name: __MMIX_call_stub_0
# OBJ-NOT:  Name: __MMIX_call_stub_1
# OBJ:      Name: chosen
# OBJ:      There are no group sections in the file.
# DIS-LABEL: <_start>:
# DIS:       GETA r1, {{[0-9]+}}
# DIS:       PUSHJ r2, {{[0-9]+}}
# DIS-LABEL: <chosen>:
# DIS:       PUSHJ r3, {{[0-9]+}}
# DIS-LABEL: <__MMIX_call_stub_0>:

#--- use.s
.section .text.start,"ax",@progbits
.global _start
_start:
  GETA r1, %geta(chosen)
  PUSHJ r2, chosen

#--- first.s
.section .text.chosen,"axG",@progbits,chosen,comdat
.global chosen
.type chosen,@function
chosen:
  PUSHJ r3, first_far
.set first_far, 0x1122334455667788

#--- second.s
.section .text.chosen,"axG",@progbits,chosen,comdat
.global chosen
.type chosen,@function
chosen:
losing_marker:
  PUSHJ r4, second_far
.set second_far, 0x8877665544332211
