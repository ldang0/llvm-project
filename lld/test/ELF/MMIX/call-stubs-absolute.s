# REQUIRES: mmix

# RUN: rm -rf %t && split-file %s %t
# RUN: llvm-mc -triple=mmix -filetype=obj %t/calls.s -o %t/calls.o
# RUN: ld.lld -T %t/layout.lds %t/calls.o -o %t/executable
# RUN: llvm-readobj --sections --symbols %t/executable | FileCheck %s --check-prefix=STRUCTURE
# RUN: llvm-objdump --no-print-imm-hex -d %t/executable | FileCheck %s --check-prefix=DIS
# RUN: llvm-objdump -s --section=.text %t/executable | FileCheck %s --check-prefix=BYTES

# STRUCTURE:      Name: .text
# STRUCTURE:      Size: 72
# STRUCTURE:      Name: __MMIX_call_stub_0
# STRUCTURE-NEXT: Value: 0x1020
# STRUCTURE-NEXT: Size: 20
# STRUCTURE:      Name: __MMIX_call_stub_1
# STRUCTURE-NEXT: Value: 0x1034
# STRUCTURE-NEXT: Size: 20

# DIS-LABEL: <_start>:
# DIS-NEXT: {{.*}} PUSHJ r1, 8
# DIS-NEXT: {{.*}} PUSHJ r2, 7
# DIS-NEXT: {{.*}} PUSHJ r3, 11
# DIS-NEXT: {{.*}} PUSHJ r4, 10
# DIS-NEXT: {{.*}} PUSHJ r5, 4
# DIS-LABEL: <__MMIX_call_stub_0>:
# DIS-NEXT: {{.*}} SETL r255, 30600
# DIS-NEXT: {{.*}} INCML r255, 21862
# DIS-NEXT: {{.*}} INCMH r255, 13124
# DIS-NEXT: {{.*}} INCH r255, 4386
# DIS-NEXT: {{.*}} GO r255, r255, 0
# DIS-LABEL: <__MMIX_call_stub_1>:

# BYTES:      Contents of section .text:
# BYTES:      1020 e3ff7788 e6ff5566 e5ff3344 e4ff1122
# BYTES-NEXT: 1030 9fffff00 e3ff778c e6ff5566 e5ff3344
# BYTES-NEXT: 1040 e4ff1122 9fffff00

#--- calls.s
.text
.global _start
.type _start,@function
_start:
PUSHJ r1, far_target
PUSHJ r2, far_target
PUSHJ r3, far_target + 4
PUSHJ r4, far_target + 4
PUSHJ r5, far_target
SWYM 0, 0, 0
SWYM 0, 0, 0
SWYM 0, 0, 0
.size _start, .-_start
.global far_target
.set far_target, 0x1122334455667788

#--- layout.lds
ENTRY(_start)
SECTIONS { .text 0x1000 : { *(.text) } }
