# RUN: llvm-mc -triple=mmix -filetype=obj %s -o %t
# RUN: llvm-readobj --file-headers --sections --section-data --symbols \
# RUN:   --relocations %t | FileCheck %s --check-prefix=OBJ \
# RUN:     --implicit-check-not='Name: .rel' --implicit-check-not='Name: .rela'
# RUN: llvm-objdump --no-print-imm-hex -d %t \
# RUN:   | FileCheck %s --check-prefix=DIS --implicit-check-not='<unknown>'

.file "elf-object-inspection.s"

.text
.global inspection_entry
.type inspection_entry,@function
inspection_entry:
SETL r1, 2
loop:
SUBU r1, r1, 1
BNZB r1, loop
JMP done
done:
SWYM 0, 0, 0
.size inspection_entry, .-inspection_entry

.section .rodata,"a",@progbits
.p2align 3
.local inspection_constant
.type inspection_constant,@object
inspection_constant:
.quad 0x0123456789abcdef
.size inspection_constant, .-inspection_constant

.data
.p2align 3
.global inspection_data
.type inspection_data,@object
inspection_data:
.quad 0xfedcba9876543210
.size inspection_data, .-inspection_data

.bss
.p2align 3
.global inspection_buffer
.type inspection_buffer,@object
inspection_buffer:
.zero 16
.size inspection_buffer, .-inspection_buffer

# OBJ:      Format: elf64-mmix
# OBJ-NEXT: Arch: mmix
# OBJ-NEXT: AddressSize: 64bit
# OBJ:      Type: Relocatable (0x1)
# OBJ-NEXT: Machine: EM_MMIX (0x50)
# OBJ-NEXT: Version: 1
# OBJ:      Flags [ (0x0)
# OBJ-NEXT: ]

# OBJ:      Name: .text
# OBJ-NEXT: Type: SHT_PROGBITS (0x1)
# OBJ-NEXT: Flags [ (0x6)
# OBJ-NEXT:   SHF_ALLOC (0x2)
# OBJ-NEXT:   SHF_EXECINSTR (0x4)
# OBJ-NEXT: ]
# OBJ:      AddressAlignment: 4
# OBJ-NEXT: EntrySize: 0
# OBJ-NEXT: SectionData (
# OBJ-NEXT:   0000: E3010002 27010101 4B01FFFF F0000001
# OBJ-NEXT:   0010: FD000000
# OBJ-NEXT: )

# OBJ:      Name: .rodata
# OBJ-NEXT: Type: SHT_PROGBITS (0x1)
# OBJ-NEXT: Flags [ (0x2)
# OBJ-NEXT:   SHF_ALLOC (0x2)
# OBJ-NEXT: ]
# OBJ:      AddressAlignment: 8
# OBJ-NEXT: EntrySize: 0
# OBJ-NEXT: SectionData (
# OBJ-NEXT:   0000: 01234567 89ABCDEF
# OBJ-NEXT: )

# OBJ:      Name: .data
# OBJ-NEXT: Type: SHT_PROGBITS (0x1)
# OBJ-NEXT: Flags [ (0x3)
# OBJ-NEXT:   SHF_ALLOC (0x2)
# OBJ-NEXT:   SHF_WRITE (0x1)
# OBJ-NEXT: ]
# OBJ:      AddressAlignment: 8
# OBJ-NEXT: EntrySize: 0
# OBJ-NEXT: SectionData (
# OBJ-NEXT:   0000: FEDCBA98 76543210
# OBJ-NEXT: )

# OBJ:      Name: .bss
# OBJ-NEXT: Type: SHT_NOBITS (0x8)
# OBJ-NEXT: Flags [ (0x3)
# OBJ-NEXT:   SHF_ALLOC (0x2)
# OBJ-NEXT:   SHF_WRITE (0x1)
# OBJ-NEXT: ]
# OBJ:      Size: 16
# OBJ:      AddressAlignment: 8
# OBJ-NEXT: EntrySize: 0

# OBJ:      Relocations [
# OBJ-NEXT: ]

# OBJ:      Name: inspection_constant
# OBJ-NEXT: Value: 0x0
# OBJ-NEXT: Size: 8
# OBJ-NEXT: Binding: Local (0x0)
# OBJ-NEXT: Type: Object (0x1)
# OBJ-NEXT: Other: 0
# OBJ-NEXT: Section: .rodata
# OBJ:      Name: inspection_entry
# OBJ-NEXT: Value: 0x0
# OBJ-NEXT: Size: 20
# OBJ-NEXT: Binding: Global (0x1)
# OBJ-NEXT: Type: Function (0x2)
# OBJ-NEXT: Other: 0
# OBJ-NEXT: Section: .text
# OBJ:      Name: inspection_data
# OBJ-NEXT: Value: 0x0
# OBJ-NEXT: Size: 8
# OBJ-NEXT: Binding: Global (0x1)
# OBJ-NEXT: Type: Object (0x1)
# OBJ-NEXT: Other: 0
# OBJ-NEXT: Section: .data
# OBJ:      Name: inspection_buffer
# OBJ-NEXT: Value: 0x0
# OBJ-NEXT: Size: 16
# OBJ-NEXT: Binding: Global (0x1)
# OBJ-NEXT: Type: Object (0x1)
# OBJ-NEXT: Other: 0
# OBJ-NEXT: Section: .bss

# DIS:      file format elf64-mmix
# DIS:      Disassembly of section .text:
# DIS:      <inspection_entry>:
# DIS-NEXT: {{.*}} SETL r1, 2
# DIS:      <loop>:
# DIS-NEXT: {{.*}} SUBU r1, r1, 1
# DIS-NEXT: {{.*}} BNZB r1, -1
# DIS-NEXT: {{.*}} JMP 1
# DIS:      <done>:
# DIS-NEXT: {{.*}} SWYM 0, 0, 0
