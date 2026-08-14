# REQUIRES: mmix

# RUN: rm -rf %t && split-file %s %t
# RUN: yaml2obj %t/input.yaml -o %t/input.o
# RUN: ld.lld -e 0 -T %t/layout.lds %t/input.o -o %t/output
# RUN: llvm-objdump -s --section=.text %t/output | FileCheck %s
# RUN: llvm-objdump --no-print-imm-hex -d %t/output \
# RUN:   | FileCheck %s --check-prefix=DIS

# CHECK:      Contents of section .text:
# CHECK-NEXT: 3000 f20a0001 fd000000 fd000000 fd000000
# CHECK-NEXT: 3010 fd000000 e3ff7788 e6ff5566 e5ff3344
# CHECK-NEXT: 3020 e4ff1122 bf0bff00
# DIS: PUSHJ r10, 1
# DIS: SETL r255, 30600
# DIS: PUSHGO r11, r255, 0

#--- layout.lds
SECTIONS { .text 0x3000 : { *(.text) } }

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
    Content:      F20A0000FD000000FD000000FD000000FD000000F20B0000FD000000FD000000FD000000FD000000
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - { Offset: 0,  Type: R_MMIX_PUSHJ, Symbol: local, Addend: 4 }
      - { Offset: 20, Type: R_MMIX_PUSHJ, Symbol: hidden_far, Addend: 8 }
Symbols:
  - { Name: local, Index: SHN_ABS, Value: 0x3000 }
  - { Name: hidden_far, Index: SHN_ABS, Value: 0x1122334455667780, Binding: STB_GLOBAL, Other: [ STV_HIDDEN ] }
