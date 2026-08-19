# REQUIRES: mmix

# RUN: yaml2obj %s -o %t.o
# RUN: not ld.lld --error-limit=0 -e 0 %t.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s

# CHECK-DAG: relocation R_MMIX_CBRANCH against symbol unaligned has a target that is not 4-byte aligned: 0x1003
# CHECK-DAG: relocation R_MMIX_PUSHJ against symbol unaligned has a target that is not 4-byte aligned: 0x1003
# CHECK-DAG: relocation R_MMIX_JMP against symbol unaligned has a target that is not 4-byte aligned: 0x1003

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
    Content:      40010000FD000000FD000000FD000000FD000000FD000000F2010000FD000000FD000000FD000000FD000000F0000000FD000000FD000000FD000000FD000000
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - { Offset: 0,  Type: R_MMIX_CBRANCH, Symbol: unaligned, Addend: 3 }
      - { Offset: 24, Type: R_MMIX_PUSHJ,   Symbol: unaligned, Addend: 3 }
      - { Offset: 44, Type: R_MMIX_JMP,     Symbol: unaligned, Addend: 3 }
Symbols:
  - { Name: unaligned, Index: SHN_ABS, Value: 0x1000 }
