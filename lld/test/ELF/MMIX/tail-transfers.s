# REQUIRES: mmix

# RUN: rm -rf %t && split-file %s %t
# RUN: llvm-mc -triple=mmix-unknown-elf -filetype=obj %t/main.s -o %t/main.o
# RUN: llvm-mc -triple=mmix-unknown-elf -filetype=obj %t/targets.s \
# RUN:   -o %t/targets.o
# RUN: llvm-readobj --relocations --expand-relocs %t/main.o \
# RUN:   | FileCheck %s --check-prefix=INPUT \
# RUN:     --implicit-check-not=R_MMIX_PUSHJ
# RUN: ld.lld --gc-sections -T %t/layout.lds %t/main.o %t/targets.o \
# RUN:   -o %t/linked
# RUN: llvm-readobj --sections --symbols --relocations %t/linked \
# RUN:   | FileCheck %s --check-prefix=LINKED \
# RUN:     --implicit-check-not=.text.dead --implicit-check-not=dead_target
# RUN: llvm-objdump --no-print-imm-hex -d %t/linked \
# RUN:   | FileCheck %s --check-prefix=DIS
# RUN: llvm-mc -triple=mmix-unknown-elf -filetype=obj %t/range.s \
# RUN:   -o %t/range.o
# RUN: ld.lld -T %t/range-near.lds %t/range.o -o %t/range-near
# RUN: llvm-objdump --no-print-imm-hex -d %t/range-near \
# RUN:   | FileCheck %s --check-prefix=RANGE-NEAR
# RUN: not ld.lld -T %t/range-far.lds %t/range.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=RANGE-FAR

# INPUT:      Section {{.*}} .rela.text.entry {
# INPUT:      Offset: 0x8
# INPUT-NEXT: Type: R_MMIX_ADDR27
# INPUT-NEXT: Symbol: external_direct
# INPUT-NEXT: Addend: 0x0
# INPUT:      Offset: 0xC
# INPUT-NEXT: Type: R_MMIX_GETA
# INPUT-NEXT: Symbol: external_indirect
# INPUT-NEXT: Addend: 0x0

# LINKED:      Name: .text.entry
# LINKED:      Name: .text.direct
# LINKED:      Name: .text.indirect
# LINKED:      Relocations [
# LINKED-NEXT: ]
# LINKED:      Name: external_direct
# LINKED:      Section: .text.direct
# LINKED:      Name: external_indirect
# LINKED:      Section: .text.indirect

# DIS-LABEL: <_start>:
# DIS:       JMP 2
# DIS-LABEL: <local_backward>:
# DIS:       JMPB -1
# DIS-LABEL: <local_forward>:
# DIS:       JMP 6
# DIS-NEXT:  GETA r250, 6
# DIS:       GO r255, r250, 0

# RANGE-NEAR-LABEL: <range_start>:
# RANGE-NEAR: JMP 1024
# RANGE-FAR: relocation R_MMIX_ADDR27 out of range: 67108864 is not in [-67108864, 67108860]

#--- main.s
.section .text.entry,"ax",@progbits
.globl _start
.type _start,@function
_start:
  JMP local_forward
local_backward:
  JMPB _start
local_forward:
  JMP external_direct
  GETA r250, %geta(external_indirect)
  GO r255, r250, 0
.size _start, .-_start

#--- targets.s
.section .text.direct,"ax",@progbits
.globl external_direct
.type external_direct,@function
external_direct:
  JMP external_direct
.size external_direct, .-external_direct

.section .text.indirect,"ax",@progbits
.globl external_indirect
.type external_indirect,@function
external_indirect:
  JMP external_indirect
.size external_indirect, .-external_indirect

.section .text.dead,"ax",@progbits
.globl dead_target
.type dead_target,@function
dead_target:
  JMP dead_target
.size dead_target, .-dead_target

#--- layout.lds
ENTRY(_start)
SECTIONS {
  .text.entry 0x1000 : { *(.text.entry) }
  .text.direct : { *(.text.direct) }
  .text.indirect : { *(.text.indirect) }
  .text.dead : { *(.text.dead) }
}

#--- range.s
.section .text.range.start,"ax",@progbits
.globl range_start
.type range_start,@function
range_start:
  JMP range_target
.size range_start, .-range_start

.section .text.range.target,"ax",@progbits
.globl range_target
.type range_target,@function
range_target:
  JMP range_target
.size range_target, .-range_target

#--- range-near.lds
SECTIONS {
  .text.range.start 0x1000 : { *(.text.range.start) }
  .text.range.target 0x2000 : { *(.text.range.target) }
}

#--- range-far.lds
SECTIONS {
  .text.range.start 0x1000 : { *(.text.range.start) }
  .text.range.target 0x4001000 : { *(.text.range.target) }
}
