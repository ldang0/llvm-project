## Test canonical MMIX disassembly and relocation annotation on a
## target-produced ELF object.

# RUN: llvm-mc -triple=mmix -filetype=obj %s -o %t.o
# RUN: llvm-objdump --no-print-imm-hex -dr %t.o 2>&1 \
# RUN:   | FileCheck %s --check-prefix=DIS --implicit-check-not='<unknown>' \
# RUN:     --implicit-check-not=warning: --implicit-check-not=error:
# RUN: llvm-objdump -r %t.o 2>&1 \
# RUN:   | FileCheck %s --check-prefix=RELOCS --strict-whitespace \
# RUN:     --match-full-lines --implicit-check-not=warning: \
# RUN:     --implicit-check-not=error:

# DIS:      file format elf64-mmix
# DIS:      Disassembly of section .text:
# DIS-LABEL: <inspection_entry>:
# DIS-NEXT:  0: e3 01 00 02   SETL r1, 2
# DIS-LABEL: <local_loop>:
# DIS-NEXT:  4: 27 01 01 01   SUBU r1, r1, 1
# DIS-NEXT:  8: 4b 01 ff ff   BNZB r1, -1
# DIS-NEXT:  c: f0 00 00 01   JMP 1
# DIS-LABEL: <local_done>:
# DIS-NEXT:  10: f4 04 00 00   GETA r4, 0
# DIS-NEXT:  {{.*}} R_MMIX_GETA external_data+0x8
# DIS-NEXT:  14: fd 00 00 00   SWYM 0, 0, 0
# DIS-NEXT:  18: fd 00 00 00   SWYM 0, 0, 0
# DIS-NEXT:  1c: fd 00 00 00   SWYM 0, 0, 0
# DIS-NEXT:  20: 40 05 00 00   BN r5, 0
# DIS-NEXT:  {{.*}} R_MMIX_ADDR19 external_branch+0x4
# DIS-NEXT:  24: f0 00 00 00   JMP 0
# DIS-NEXT:  {{.*}} R_MMIX_ADDR27 external_jump-0x8
# DIS-NEXT:  28: f2 06 00 00   PUSHJ r6, 0
# DIS-NEXT:  {{.*}} R_MMIX_PUSHJ_STUBBABLE external_call+0xc
# DIS-NEXT:  2c: f2 07 00 02   PUSHJ r7, 2
# DIS-NEXT:  30: fd 01 02 03   SWYM 1, 2, 3
# DIS-LABEL: <local_call>:
# DIS-NEXT:  34: fd 00 00 00   SWYM 0, 0, 0

#      RELOCS:RELOCATION RECORDS FOR [.text]:
# RELOCS-NEXT:OFFSET           TYPE                     VALUE
# RELOCS-NEXT:0000000000000010 R_MMIX_GETA              external_data+0x8
# RELOCS-NEXT:0000000000000020 R_MMIX_ADDR19            external_branch+0x4
# RELOCS-NEXT:0000000000000024 R_MMIX_ADDR27            external_jump-0x8
# RELOCS-NEXT:0000000000000028 R_MMIX_PUSHJ_STUBBABLE   external_call+0xc
# RELOCS-EMPTY:
# RELOCS-NEXT:RELOCATION RECORDS FOR [.data]:
# RELOCS-NEXT:OFFSET           TYPE                     VALUE
# RELOCS-NEXT:0000000000000000 R_MMIX_8                 external_data+0x1
# RELOCS-NEXT:0000000000000002 R_MMIX_16                external_data-0x2
# RELOCS-NEXT:0000000000000004 R_MMIX_32                external_data+0x3
# RELOCS-NEXT:0000000000000008 R_MMIX_64                external_data-0x4

.text
.global inspection_entry
.type inspection_entry,@function
inspection_entry:
SETL r1, 2
local_loop:
SUBU r1, r1, 1
BNZB r1, local_loop
JMP local_done
local_done:
GETA r4, %geta(external_data + 8)
BN r5, external_branch + 4
JMP external_jump - 8
PUSHJ r6, external_call + 12
PUSHJ r7, local_call
SWYM 1, 2, 3
local_call:
SWYM 0, 0, 0
.size inspection_entry, .-inspection_entry

.global external_data
.global external_branch
.global external_jump
.global external_call
.type external_call,@function

.data
.byte external_data + 1
.p2align 1
.short external_data - 2
.p2align 2
.long external_data + 3
.p2align 3
.quad external_data - 4
