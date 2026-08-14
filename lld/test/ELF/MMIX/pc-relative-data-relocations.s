# REQUIRES: mmix

# RUN: echo 'SECTIONS { .text 0x1000 : { *(.text) } \
# RUN:                    .data 0x2000 : { *(.data) } }' > %t.script
# RUN: yaml2obj --docnum=1 %s -o %t.o
# RUN: ld.lld -e 0 -T %t.script %t.o -o %t
# RUN: llvm-objdump -s --section=.data %t | FileCheck %s --check-prefix=DATA
# RUN: yaml2obj --docnum=2 %s -o %t-overflow.o
# RUN: not ld.lld -e 0 -T %t.script %t-overflow.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=OVERFLOW

## Every result depends on P=0x2000+Offset, so this also checks that scripted
## output placement and the individual relocation-field address determine P.
## The first eight fields are the accepted positive and negative GNU bitfield
## boundaries. R_MMIX_PC_24 preserves A5 and 5A in the high storage byte.
# DATA:      Contents of section .data:
# DATA-NEXT: 2000 ff00ffff 0000a5ff ffff5a00 0000ffff
# DATA-NEXT: 2010 ffff0000 0000ffff ffffffff e00a0000
# DATA-NEXT: 2020 00000000 0000

# OVERFLOW-DAG: relocation R_MMIX_PC_8 out of range: 256 is not in [-256, 255]
# OVERFLOW-DAG: relocation R_MMIX_PC_8 out of range: -257 is not in [-256, 255]
# OVERFLOW-DAG: relocation R_MMIX_PC_16 out of range: 65536 is not in [-65536, 65535]
# OVERFLOW-DAG: relocation R_MMIX_PC_16 out of range: -65537 is not in [-65536, 65535]
# OVERFLOW-DAG: relocation R_MMIX_PC_24 out of range: 16777216 is not in [-16777216, 16777215]
# OVERFLOW-DAG: relocation R_MMIX_PC_24 out of range: -16777217 is not in [-16777216, 16777215]
# OVERFLOW-DAG: relocation R_MMIX_PC_32 out of range: 4294967296 is not in [-4294967296, 4294967295]
# OVERFLOW-DAG: relocation R_MMIX_PC_32 out of range: -4294967297 is not in [-4294967296, 4294967295]

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
    AddressAlign: 4
    Content:      FD000000
  - Name:         .data
    Type:         SHT_PROGBITS
    Flags:        [ SHF_ALLOC, SHF_WRITE ]
    AddressAlign: 1
    Content:      000000000000A5FFFFFF5AFFFFFF000000000000000000000000000000000000000000000000
  - Name: .rela.data
    Type: SHT_RELA
    Link: .symtab
    Info: .data
    Relocations:
      - { Offset: 0,  Type: R_MMIX_PC_8,  Symbol: pos8, Addend: 0 }
      - { Offset: 1,  Type: R_MMIX_PC_8,  Symbol: neg8, Addend: 0 }
      - { Offset: 2,  Type: R_MMIX_PC_16, Symbol: pos16, Addend: 16 }
      - { Offset: 4,  Type: R_MMIX_PC_16, Symbol: neg16, Addend: 0 }
      - { Offset: 6,  Type: R_MMIX_PC_24, Symbol: pos24, Addend: 0 }
      - { Offset: 10, Type: R_MMIX_PC_24, Symbol: neg24, Addend: 0 }
      - { Offset: 14, Type: R_MMIX_PC_32, Symbol: pos32, Addend: 0 }
      - { Offset: 18, Type: R_MMIX_PC_32, Symbol: neg32, Addend: 0 }
      - { Offset: 22, Type: R_MMIX_PC_64, Symbol: wrap64, Addend: 48 }
      - { Offset: 30, Type: R_MMIX_PC_64, Symbol: zero64, Addend: 0 }
Symbols:
  - { Name: pos8,   Index: SHN_ABS, Value: 0x20ff }
  - { Name: neg8,   Index: SHN_ABS, Value: 0x1f01 }
  - { Name: pos16,  Index: SHN_ABS, Value: 0x11ff1 }
  - { Name: neg16,  Index: SHN_ABS, Value: 0xffffffffffff2004 }
  - { Name: pos24,  Index: SHN_ABS, Value: 0x1002005 }
  - { Name: neg24,  Index: SHN_ABS, Value: 0xffffffffff00200a }
  - { Name: pos32,  Index: SHN_ABS, Value: 0x10000200d }
  - { Name: neg32,  Index: SHN_ABS, Value: 0xffffffff00002012 }
  - { Name: wrap64, Index: SHN_ABS, Value: 0xfffffffffffffff0 }
  - { Name: zero64, Index: SHN_ABS, Value: 0x201e }

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
    AddressAlign: 4
    Content:      FD000000
  - Name:         .data
    Type:         SHT_PROGBITS
    Flags:        [ SHF_ALLOC, SHF_WRITE ]
    AddressAlign: 1
    Content:      00000000000000000000000000000000000000000000
  - Name: .rela.data
    Type: SHT_RELA
    Link: .symtab
    Info: .data
    Relocations:
      - { Offset: 0,  Type: R_MMIX_PC_8,  Symbol: zero, Addend: 0x2100 }
      - { Offset: 1,  Type: R_MMIX_PC_8,  Symbol: zero, Addend: 0x1f00 }
      - { Offset: 2,  Type: R_MMIX_PC_16, Symbol: zero, Addend: 0x12002 }
      - { Offset: 4,  Type: R_MMIX_PC_16, Symbol: zero, Addend: -57341 }
      - { Offset: 6,  Type: R_MMIX_PC_24, Symbol: zero, Addend: 0x1002006 }
      - { Offset: 10, Type: R_MMIX_PC_24, Symbol: zero, Addend: -16769015 }
      - { Offset: 14, Type: R_MMIX_PC_32, Symbol: zero, Addend: 0x10000200e }
      - { Offset: 18, Type: R_MMIX_PC_32, Symbol: zero, Addend: -4294959087 }
Symbols:
  - { Name: zero, Index: SHN_ABS, Value: 0 }
