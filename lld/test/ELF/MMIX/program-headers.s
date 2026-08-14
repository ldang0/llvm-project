# REQUIRES: mmix

# RUN: yaml2obj %s -o %t.o
# RUN: ld.lld %t.o -o %t
# RUN: llvm-readobj --elf-output-style=GNU --sections --program-headers %t \
# RUN:   | FileCheck %s

## MMIX inherits lld's generic static image and page layout. Check the
## resulting load permissions, zero-fill storage, alignments, and mappings
## without prescribing a platform memory map.

# CHECK:      Section Headers:
# CHECK:      Name              Type            Address          Off    Size
# CHECK:      .rodata           PROGBITS        {{[0-9a-f]+}} {{[0-9a-f]+}} 000008 {{.*}} A  {{.*}} 32
# CHECK:      .text             PROGBITS        {{[0-9a-f]+}} {{[0-9a-f]+}} 000008 {{.*}} AX {{.*}} 16
# CHECK:      .data             PROGBITS        {{[0-9a-f]+}} {{[0-9a-f]+}} 000008 {{.*}} WA {{.*}} 64
# CHECK:      .bss              NOBITS          {{[0-9a-f]+}} {{[0-9a-f]+}} 000020 {{.*}} WA {{.*}} 128
# CHECK:      .note.mmix        NOTE            {{0+}} {{[0-9a-f]+}} 000004 {{.*}} 0   0  4

# CHECK:      There are 5 program headers
# CHECK:      Type           Offset   VirtAddr           PhysAddr           FileSiz  MemSiz   Flg Align
# CHECK-NEXT: PHDR           0x000040 0x0000000000010040 0x0000000000010040 0x000118 0x000118 R   0x8
# CHECK-NEXT: LOAD           0x000000 0x0000000000010000 0x0000000000010000 0x000168 0x000168 R   0x1000
# CHECK-NEXT: LOAD           0x000170 0x0000000000011170 0x0000000000011170 0x000008 0x000008 R E 0x1000
# CHECK-NEXT: LOAD           0x000180 0x0000000000012180 0x0000000000012180 0x000008 0x0000a0 RW  0x1000
# CHECK-NEXT: GNU_STACK      0x000000 0x0000000000000000 0x0000000000000000 0x000000 0x000000 RW  0x0

## The offsets and virtual addresses above are congruent modulo p_align.
## The writable segment has p_filesz < p_memsz because .bss is zero-filled.
# CHECK:      Section to Segment mapping:
# CHECK-NEXT: Segment Sections...
# CHECK-NEXT: 00
# CHECK-NEXT: 01     .rodata
# CHECK-NEXT: 02     .text
# CHECK-NEXT: 03     .data .bss
# CHECK-NEXT: 04
# CHECK-NEXT: None   .note.mmix

--- !ELF
FileHeader:
  Class:   ELFCLASS64
  Data:    ELFDATA2MSB
  Type:    ET_REL
  Machine: EM_MMIX
Sections:
  - Name:         .text
    Type:         SHT_PROGBITS
    Flags:        [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 16
    Content:      FD000000FD000000
  - Name:         .rodata
    Type:         SHT_PROGBITS
    Flags:        [ SHF_ALLOC ]
    AddressAlign: 32
    Content:      0123456789ABCDEF
  - Name:         .data
    Type:         SHT_PROGBITS
    Flags:        [ SHF_ALLOC, SHF_WRITE ]
    AddressAlign: 64
    Content:      FEDCBA9876543210
  - Name:         .bss
    Type:         SHT_NOBITS
    Flags:        [ SHF_ALLOC, SHF_WRITE ]
    AddressAlign: 128
    Size:         32
  - Name:         .note.mmix
    Type:         SHT_NOTE
    AddressAlign: 4
    Content:      00000000
Symbols:
  - Name:    _start
    Type:    STT_FUNC
    Section: .text
    Binding: STB_GLOBAL
