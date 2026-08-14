# REQUIRES: mmix

## Build a deterministic, self-contained static executable from multiple
## LLVM-produced objects. It deliberately needs no runtime or dynamic linker.
# RUN: rm -rf %t && split-file %s %t
# RUN: llvm-mc -triple=mmix -filetype=obj %t/start.s -o %t/start.o
# RUN: llvm-mc -triple=mmix -filetype=obj %t/worker.s -o %t/worker.o
# RUN: llvm-mc -triple=mmix -filetype=obj %t/data.s -o %t/data.o
# RUN: ld.lld -static -T %t/layout.lds %t/start.o %t/worker.o %t/data.o \
# RUN:   -o %t/executable.1
# RUN: ld.lld -static -T %t/layout.lds %t/start.o %t/worker.o %t/data.o \
# RUN:   -o %t/executable.2
# RUN: cmp %t/executable.1 %t/executable.2
# RUN: llvm-readobj --file-headers --sections --symbols --program-headers \
# RUN:   --relocations %t/executable.1 | FileCheck %s --check-prefix=STRUCTURE \
# RUN:   --implicit-check-not=.dynamic --implicit-check-not=.dynsym \
# RUN:   --implicit-check-not=R_MMIX_
# RUN: llvm-objdump --no-print-imm-hex -d %t/executable.1 \
# RUN:   | FileCheck %s --check-prefix=DIS
# RUN: llvm-objdump -s --section=.data %t/executable.1 \
# RUN:   | FileCheck %s --check-prefix=DATA

## Keep both stable next-milestone failures beside the positive baseline.
# RUN: llvm-mc -triple=mmix -filetype=obj %t/milestone3.s -o %t/milestone3.o
# RUN: not ld.lld --error-limit=0 -e unsupported_entry %t/milestone3.o \
# RUN:   %t/worker.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=MILESTONE3

# STRUCTURE:      Format: elf64-mmix
# STRUCTURE:      Type: Executable
# STRUCTURE:      Entry: 0x10000
# STRUCTURE:      Name: .text
# STRUCTURE:      Address: 0x10000
# STRUCTURE:      Name: .data
# STRUCTURE:      Address: 0x20000
# STRUCTURE:      ProgramHeaders [
# STRUCTURE:      Type: PT_LOAD
# STRUCTURE:      PF_X
# STRUCTURE:      Type: PT_LOAD
# STRUCTURE:      PF_W
# STRUCTURE:      Name: _start
# STRUCTURE:      Value: 0x10000
# STRUCTURE:      Name: worker
# STRUCTURE:      Value: 0x10004
# STRUCTURE:      Name: state
# STRUCTURE:      Value: 0x20008

# DIS-LABEL: <_start>:
# DIS-NEXT:  {{.*}} JMP 1
# DIS-LABEL: <worker>:
# DIS-NEXT:  {{.*}} BNB r1, -1
# DIS-NEXT:  {{.*}} JMPB -1

# DATA:      Contents of section .data:
# DATA-NEXT: 20000 00000000 00010004 00000000 00001122
# DATA-NEXT: 20010 121234a5 00123412 34567811 22334455
# DATA-NEXT: 20020 667788e5 ffe45aff ffe2ffff ffdeffff
# DATA-NEXT: 20030 ffffffff ffda

# MILESTONE3-DAG: unsupported relocation R_MMIX_GETA against symbol state: requires MMIX relaxation support
# MILESTONE3-DAG: unsupported relocation R_MMIX_PUSHJ_STUBBABLE against symbol worker: requires MMIX range-extension stub support

#--- layout.lds
ENTRY(_start)
SECTIONS {
  .text 0x10000 : { *(.text) }
  .data 0x20000 : { *(.data) }
}

#--- start.s
.text
.global _start
.type _start,@function
_start:
JMP worker
.size _start, .-_start
.global worker
.data
.quad worker

#--- worker.s
.text
.global worker
.type worker,@function
worker:
BN r1, _start
JMP worker
.size worker, .-worker
.global _start
.data
.global state
.type state,@object
state:
.quad 0x1122
.size state, .-state

#--- data.s
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
.global state

.data
.byte abs8
.short abs16
.mmix_24 0xa5, abs24
.long abs32
.quad abs64
.Lpc8:
.byte state - .Lpc8
.Lpc16:
.short state - .Lpc16
.mmix_pc_24 0x5a, state
.Lpc32:
.long state - .Lpc32
.Lpc64:
.quad state - .Lpc64

#--- milestone3.s
.text
.global unsupported_entry
unsupported_entry:
GETA r1, %geta(state)
PUSHJ r2, worker
.global state
.global worker
