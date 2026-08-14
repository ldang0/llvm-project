# REQUIRES: mmix

# RUN: rm -rf %t && split-file %s %t
# RUN: yaml2obj %t/input.yaml -o %t/input.o
# RUN: ld.lld -e 0 -T %t/layout.lds %t/input.o -o %t/output
# RUN: llvm-objdump -s --section=.text %t/output | FileCheck %s
# RUN: llvm-objdump --no-print-imm-hex -d %t/output \
# RUN:   | FileCheck %s --check-prefix=DIS

## The direct branch preserves opcode and X. The expanded predicted BNZ
## toggles the architectural condition and prediction bits to BZ, skips six
## instructions, and transfers through GOI r255,r255,0.
# CHECK:      Contents of section .text:
# CHECK-NEXT: 2000 5a070001 fd000000 fd000000 fd000000
# CHECK-NEXT: 2010 fd000000 fd000000 42090006 e3ff7788
# CHECK-NEXT: 2020 e6ff5566 e5ff3344 e4ff1122 9fffff00
# DIS: PBNZ r7, 1
# DIS: BZ r9, 6
# DIS: SETL r255, 30600
# DIS: GO r255, r255, 0

#--- layout.lds
SECTIONS { .text 0x2000 : { *(.text) } }

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
    Content:      5A070000FD000000FD000000FD000000FD000000FD0000005A090000FD000000FD000000FD000000FD000000FD000000
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - { Offset: 0,  Type: R_MMIX_CBRANCH, Symbol: near, Addend: 4 }
      - { Offset: 24, Type: R_MMIX_CBRANCH, Symbol: far, Addend: 8 }
Symbols:
  - { Name: near, Index: SHN_ABS, Value: 0x2000 }
  - { Name: far,  Index: SHN_ABS, Value: 0x1122334455667780 }
