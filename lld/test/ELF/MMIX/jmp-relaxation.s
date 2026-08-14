# REQUIRES: mmix

# RUN: rm -rf %t && split-file %s %t
# RUN: yaml2obj %t/input.yaml -o %t/input.o
# RUN: ld.lld -e 0 -T %t/layout.lds %t/input.o -o %t/output
# RUN: llvm-objdump -s --section=.text %t/output | FileCheck %s
# RUN: llvm-objdump --no-print-imm-hex -d %t/output \
# RUN:   | FileCheck %s --check-prefix=DIS

# CHECK:      Contents of section .text:
# CHECK-NEXT: 4000 f0000001 fd000000 fd000000 fd000000
# CHECK-NEXT: 4010 fd000000 f1000000 fd000000 fd000000
# CHECK-NEXT: 4020 fd000000 fd000000 e3ff7788 e6ff5566
# CHECK-NEXT: 4030 e5ff3344 e4ff1122 9fffff00
# DIS: JMP 1
# DIS: JMPB -16777216
# DIS: SETL r255, 30600
# DIS: GO r255, r255, 0

#--- layout.lds
SECTIONS { .text 0x4000 : { *(.text) } }

#--- input.yaml
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
    Content:      F0000000FD000000FD000000FD000000FD000000F0000000FD000000FD000000FD000000FD000000F0000000FD000000FD000000FD000000FD000000
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - { Offset: 0,  Type: R_MMIX_JMP, Symbol: forward, Addend: 0 }
      - { Offset: 20, Type: R_MMIX_JMP, Symbol: lower, Addend: 0 }
      - { Offset: 40, Type: R_MMIX_JMP, Symbol: far, Addend: 8 }
Symbols:
  - { Name: forward, Index: SHN_ABS, Value: 0x4004 }
  - { Name: lower,   Index: SHN_ABS, Value: 0xfffffffffc004014 }
  - { Name: far,     Index: SHN_ABS, Value: 0x1122334455667780 }
