# REQUIRES: mmix

# RUN: rm -rf %t && split-file %s %t
# RUN: yaml2obj %t/valid.yaml -o %t/valid.o
# RUN: ld.lld -e 0 -T %t/layout.lds %t/valid.o -o %t/valid
# RUN: llvm-objdump -s --section=.text %t/valid | FileCheck %s
# RUN: llvm-objdump --no-print-imm-hex -d %t/valid \
# RUN:   | FileCheck %s --check-prefix=DIS
# RUN: llvm-readobj --relocations %t/valid | FileCheck %s --check-prefix=RELOCS
# RUN: yaml2obj %t/invalid.yaml -o %t/invalid.o
# RUN: not ld.lld -e 0 -T %t/layout.lds %t/invalid.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=INVALID

# CHECK:      Contents of section .text:
# CHECK-NEXT: 1000 f4010001 fd000000 fd000000 fd000000
# CHECK-NEXT: 1010 f5020000 fd000000 fd000000 fd000000
# CHECK-NEXT: 1020 f403ffff fd000000 fd000000 fd000000
# CHECK-NEXT: 1030 e3047788 e6045566 e5043344 e4041122
# CHECK-NEXT: 1040 f505fbf0 fd000000 fd000000 fd000000
# DIS: GETA r1, 1
# DIS: GETAB r2, -65536
# DIS: GETA r3, 65535
# DIS: SETL r4, 30600
# DIS: INCML r4, 21862
# DIS: INCMH r4, 13124
# DIS: INCH r4, 4386
# DIS: GETAB r5, -1040
# RELOCS:      Relocations [
# RELOCS-NEXT: ]
# INVALID: relocation R_MMIX_GETA against symbol unaligned has a target that is not 4-byte aligned: 0x1002

#--- layout.lds
SECTIONS { .text 0x1000 : { *(.text) } }

#--- valid.yaml
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
    Content:      F4010000FD000000FD000000FD000000F4020000FD000000FD000000FD000000F4030000FD000000FD000000FD000000F4040000FD000000FD000000FD000000F4050000FD000000FD000000FD000000
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - { Offset: 0,  Type: R_MMIX_GETA, Symbol: forward, Addend: 0 }
      - { Offset: 16, Type: R_MMIX_GETA, Symbol: lower, Addend: 0 }
      - { Offset: 32, Type: R_MMIX_GETA, Symbol: upper, Addend: 0 }
      - { Offset: 48, Type: R_MMIX_GETA, Symbol: far, Addend: 8 }
      - { Offset: 64, Type: R_MMIX_GETA, Symbol: weak, Addend: 0 }
Symbols:
  - { Name: forward, Index: SHN_ABS, Value: 0x1004 }
  - { Name: lower,   Index: SHN_ABS, Value: 0xfffffffffffc1010 }
  - { Name: upper,   Index: SHN_ABS, Value: 0x4101c }
  - { Name: far,     Index: SHN_ABS, Value: 0x1122334455667780 }
  - { Name: weak,    Index: SHN_UNDEF, Binding: STB_WEAK }

#--- invalid.yaml
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
    Content:      F4010000FD000000FD000000FD000000
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - { Offset: 0, Type: R_MMIX_GETA, Symbol: unaligned, Addend: 0 }
Symbols:
  - { Name: unaligned, Index: SHN_ABS, Value: 0x1002 }
