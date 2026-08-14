# REQUIRES: mmix

# RUN: rm -rf %t && split-file %s %t
# RUN: llvm-mc -triple=mmix -filetype=obj %t/calls.s -o %t/calls.o
# RUN: llvm-mc -triple=mmix -filetype=obj %t/target.s -o %t/target.o
# RUN: ld.lld -T %t/near.lds %t/calls.o %t/target.o -o %t/near
# RUN: llvm-objdump --no-print-imm-hex -d %t/near | FileCheck %s --check-prefix=NEAR
# RUN: ld.lld -T %t/stub.lds %t/calls.o %t/target.o -o %t/stub.1
# RUN: ld.lld -T %t/stub.lds %t/calls.o %t/target.o -o %t/stub.2
# RUN: cmp %t/stub.1 %t/stub.2
# RUN: llvm-readobj --sections --symbols %t/stub.1 | FileCheck %s --check-prefix=STRUCTURE
# RUN: llvm-objdump --no-print-imm-hex -d %t/stub.1 | FileCheck %s --check-prefix=STUB
# RUN: ld.lld -T %t/backward.lds %t/calls.o %t/target.o -o %t/backward
# RUN: llvm-objdump --no-print-imm-hex -d %t/backward | FileCheck %s --check-prefix=BACKWARD
# RUN: llvm-mc -triple=mmix -filetype=obj %t/boundaries.s -o %t/boundaries.o
# RUN: ld.lld -T %t/boundaries.lds %t/boundaries.o -o %t/boundaries
# RUN: llvm-objdump --no-print-imm-hex -d %t/boundaries | FileCheck %s --check-prefix=BOUNDARY

# NEAR-LABEL: <_start>:
# NEAR-NEXT: {{.*}} PUSHJ r1, 3
# NEAR-NEXT: {{.*}} PUSHJ r2, 2
# NEAR-NEXT: {{.*}} PUSHJ r3, 1
# NEAR-NOT: __MMIX_call_stub_

# STRUCTURE:      Name: .text
# STRUCTURE:      Size: 16
# STRUCTURE:      Name: __MMIX_call_stub_0
# STRUCTURE-NEXT: Value: 0x100C
# STRUCTURE-NEXT: Size: 4
# STRUCTURE:      Binding: Local
# STRUCTURE:      Type: Function
# STRUCTURE-NOT:  Name: __MMIX_call_stub_1

# STUB-LABEL: <_start>:
# STUB-NEXT: {{.*}} PUSHJ r1, 3
# STUB-NEXT: {{.*}} PUSHJ r2, 2
# STUB-NEXT: {{.*}} PUSHJ r3, 1
# STUB-LABEL: <__MMIX_call_stub_0>:
# STUB-NEXT: {{.*}} JMP 523261

# BACKWARD-LABEL: <_start>:
# BACKWARD-NEXT: {{.*}} PUSHJ r1, 3
# BACKWARD-LABEL: <__MMIX_call_stub_0>:
# BACKWARD-NEXT: {{.*}} JMPB -523267

# BOUNDARY-LABEL: <boundary_calls>:
# BOUNDARY-NEXT: {{.*}} PUSHJ r1, 65535
# BOUNDARY-NEXT: {{.*}} PUSHJB r2, -65536
# BOUNDARY-NOT: __MMIX_call_stub_

#--- calls.s
.text
.global _start
.type _start,@function
_start:
PUSHJ r1, target
PUSHJ r2, target
PUSHJ r3, target
.size _start, .-_start
.global target

#--- target.s
.section .target,"ax",@progbits
.global target
.type target,@function
target:
POP 0, 0
.size target, .-target

#--- near.lds
ENTRY(_start)
SECTIONS {
  .text 0x1000 : { *(.text) *(.target) }
}

#--- stub.lds
ENTRY(_start)
SECTIONS {
  .text 0x1000 : { *(.text) }
  .target 0x200000 : { *(.target) }
}

#--- backward.lds
ENTRY(_start)
SECTIONS {
  .target 0x1000 : { *(.target) }
  .text 0x200000 : { *(.text) }
}

#--- boundaries.s
.text
.global boundary_calls
boundary_calls:
PUSHJ r1, upper
PUSHJ r2, lower
.global upper
.set upper, 0x40ffc
.global lower
.set lower, 0xfffffffffffc1004

#--- boundaries.lds
ENTRY(boundary_calls)
SECTIONS { .text 0x1000 : { *(.text) } }
