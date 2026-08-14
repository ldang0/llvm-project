# REQUIRES: mmix

## Link relocatable objects produced by llvm-mc, covering every relocation
## family implemented in the first two MMIX linker milestones.
# RUN: rm -rf %t && split-file %s %t
# RUN: llvm-mc -triple=mmix -filetype=obj %t/use.s -o %t/use.o
# RUN: llvm-mc -triple=mmix -filetype=obj %t/definitions.s -o %t/definitions.o
# RUN: ld.lld -T %t/layout.lds %t/use.o %t/definitions.o -o %t/executable
# RUN: llvm-readobj --file-headers --sections --symbols %t/executable \
# RUN:   | FileCheck %s --check-prefix=STRUCTURE
# RUN: llvm-readobj --relocations %t/executable \
# RUN:   | FileCheck %s --check-prefix=RELOCS --implicit-check-not=R_MMIX_
# RUN: llvm-objdump --no-print-imm-hex -d %t/executable \
# RUN:   | FileCheck %s --check-prefix=DIS
# RUN: llvm-objdump -s --section=.data %t/executable \
# RUN:   | FileCheck %s --check-prefix=DATA

# STRUCTURE:      Format: elf64-mmix
# STRUCTURE:      Type: Executable
# STRUCTURE:      Entry: 0x10000
# STRUCTURE:      Name: .text
# STRUCTURE:      Address: 0x10000
# STRUCTURE:      Name: .data
# STRUCTURE:      Address: 0x20000
# STRUCTURE:      Name: _start
# STRUCTURE:      Value: 0x10000
# STRUCTURE:      Name: branch_target
# STRUCTURE:      Value: 0x10010
# STRUCTURE:      Name: data_target
# STRUCTURE:      Section: .data
# RELOCS:      Relocations [
# RELOCS-NEXT: ]

# DIS-LABEL: <_start>:
# DIS-NEXT:  {{.*}} BN r1, 5
# DIS-NEXT:  {{.*}} JMP 3
# DIS-NEXT:  {{.*}} JMP 1
# DIS-LABEL: <local_target>:
# DIS-NEXT:  {{.*}} SWYM 0, 0, 0
# DIS-LABEL: <branch_target>:

# DATA:      Contents of section .data:
# DATA-NEXT: 20000 131236a5 00123712 34567c11 22334455
# DATA-NEXT: 20010 66778d13 00125a00 00100000 000c0000
# DATA-NEXT: 20020 00000000 00080000 00000000 cafe

#--- layout.lds
ENTRY(_start)
SECTIONS {
  .text 0x10000 : { *(.text) }
  .data 0x20000 : { *(.data) }
}

#--- use.s
.text
.global _start
.type _start,@function
_start:
BN r1, branch_target + 4
JMP jump_target - 4
JMP local_target
local_target:
SWYM 0, 0, 0
.size _start, .-_start
.global branch_target
.global jump_target

.data
.byte abs8 + 1
.short abs16 + 2
.mmix_24 0xa5, abs24 + 3
.long abs32 + 4
.quad abs64 + 5
.Lpc8:
.byte data_target - .Lpc8
.Lpc16:
.short data_target - .Lpc16
.mmix_pc_24 0x5a, data_target
.Lpc32:
.long data_target - .Lpc32
.Lpc64:
.quad data_target - .Lpc64
.global abs8
.global abs16
.global abs24
.global abs32
.global abs64
.global data_target

#--- definitions.s
.global abs8
.set abs8, 0x12
.global abs16
.set abs16, 0x1234
.global abs24
.set abs24, 0x1234
.global abs32
.set abs32, 0x12345678
.global abs64
.set abs64, 0x1122334455667788

.text
.global branch_target
branch_target:
SWYM 0, 0, 0
.global jump_target
jump_target:
SWYM 0, 0, 0

.data
.global data_target
data_target:
.quad 0xcafe
