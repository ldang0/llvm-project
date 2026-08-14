# REQUIRES: mmix

# RUN: rm -rf %t && split-file %s %t
# RUN: llvm-mc -triple=mmix -filetype=obj %t/primary.s -o %t/primary.o
# RUN: llvm-mc -triple=mmix -filetype=obj %t/large-call.s -o %t/large-call.o
# RUN: llvm-mc -triple=mmix -filetype=obj %t/target.s -o %t/target.o
# RUN: ld.lld --verbose -T %t/relax.lds %t/primary.o %t/large-call.o %t/target.o -o %t/relax 2>&1 | FileCheck %s --check-prefix=PASSES
# RUN: llvm-objdump --no-print-imm-hex -d --start-address=0x1000 --stop-address=0x1010 %t/relax | FileCheck %s --check-prefix=RELAX
# RUN: llvm-readobj --symbols %t/relax | FileCheck %s --check-prefix=RELAX-SYMS

# RUN: llvm-mc -triple=mmix -filetype=obj %t/gc.s -o %t/gc.o
# RUN: ld.lld --gc-sections -e live -T %t/gc.lds %t/gc.o -o %t/gc
# RUN: llvm-readobj --sections --symbols %t/gc | FileCheck %s --check-prefix=GC
# RUN: llvm-objdump --no-print-imm-hex -d %t/gc | FileCheck %s --check-prefix=GC-DIS

# RUN: llvm-mc -triple=mmix -filetype=obj %t/a.s -o %t/a.o
# RUN: llvm-mc -triple=mmix -filetype=obj %t/b.s -o %t/b.o
# RUN: ld.lld -T %t/domains.lds %t/a.o %t/b.o -o %t/domains.ab
# RUN: ld.lld -T %t/domains.lds %t/b.o %t/a.o -o %t/domains.ba
# RUN: llvm-objdump -s --section=.text.a --section=.text.b %t/domains.ab | sed '1,2d' > %t/ab.txt
# RUN: llvm-objdump -s --section=.text.a --section=.text.b %t/domains.ba | sed '1,2d' > %t/ba.txt
# RUN: cmp %t/ab.txt %t/ba.txt
# RUN: llvm-readobj --sections --symbols %t/domains.ab | FileCheck %s --check-prefix=DOMAINS

# PASSES: MMIX relaxation passes: 3
# RELAX: SETL r1, 4112
# RELAX-NEXT: INCML r1, 4
# RELAX-NEXT: INCMH r1, 0
# RELAX-NEXT: INCH r1, 0
# RELAX-SYMS:      Name: __MMIX_call_stub_0
# RELAX-SYMS-NEXT: Value: 0x40FFC
# RELAX-SYMS-NEXT: Size: 20
# RELAX-SYMS:      Name: target
# RELAX-SYMS-NEXT: Value: 0x41010

# GC:      Name: .text.live
# GC:      Size: 24
# GC-NOT:  Name: .text.dead
# GC:      Name: __MMIX_call_stub_0
# GC-NOT:  Name: __MMIX_call_stub_1
# GC-NOT:  Name: dead
# GC-DIS-LABEL: <live>:
# GC-DIS: PUSHJ r1, 1
# GC-DIS-LABEL: <__MMIX_call_stub_0>:

# DOMAINS:      Name: .text.a
# DOMAINS:      Size: 24
# DOMAINS:      Name: .text.b
# DOMAINS:      Size: 24
# DOMAINS:      Name: __MMIX_call_stub_0
# DOMAINS:      Name: __MMIX_call_stub_1

#--- primary.s
.section .text.primary,"ax",@progbits
.global _start
.type _start,@function
_start:
GETA r1, %geta(target)
.size _start, .-_start
.global target

#--- large-call.s
.section .text.call,"ax",@progbits
.global call
.type call,@function
call:
PUSHJ r2, far_target
.space 0x3ffe8
.size call, .-call
.global far_target
.set far_target, 0x1122334455667788

#--- target.s
.section .text.target,"ax",@progbits
.global target
.type target,@function
target:
POP 0, 0
.size target, .-target

#--- relax.lds
ENTRY(_start)
SECTIONS {
  .text 0x1000 : { *(.text.primary) *(.text.call) *(.text.target) }
}

#--- gc.s
.section .text.live,"ax",@progbits
.global live
.type live,@function
live:
PUSHJ r1, far_target
.size live, .-live

.section .text.dead,"ax",@progbits
.global dead
.type dead,@function
dead:
PUSHJ r2, far_target + 4
.size dead, .-dead

.global far_target
.set far_target, 0x1122334455667788

#--- gc.lds
SECTIONS {
  .text.live 0x1000 : { *(.text.live) }
  .text.dead : { *(.text.dead) }
}

#--- a.s
.section .text.a,"ax",@progbits
.global a
a:
PUSHJ r1, far_target
.global far_target
.set far_target, 0x1122334455667788

#--- b.s
.section .text.b,"ax",@progbits
.global b
b:
PUSHJ r2, far_target
.global far_target
.set far_target, 0x1122334455667788

#--- domains.lds
ENTRY(a)
SECTIONS {
  .text.a 0x1000 : { *(.text.a) }
  .text.b 0x2000 : { *(.text.b) }
}
