# REQUIRES: mmix

# RUN: rm -rf %t && split-file %s %t
# RUN: yaml2obj %t/use.yaml -o %t/use.o
# RUN: yaml2obj %t/definitions.yaml -o %t/definitions.o
# RUN: ld.lld -e 0 %t/use.o %t/definitions.o -o %t/integrated-a
# RUN: ld.lld -e 0 %t/definitions.o %t/use.o -o %t/integrated-b
# RUN: llvm-readobj --relocations %t/integrated-a \
# RUN:   | FileCheck %s --check-prefix=RELOCS
# RUN: llvm-objdump -s --section=.text --section=.MMIX.reg_contents \
# RUN:   %t/integrated-a | FileCheck %s --check-prefix=INTEGRATED
# RUN: llvm-objdump -s --section=.text --section=.MMIX.reg_contents \
# RUN:   %t/integrated-b | FileCheck %s --check-prefix=INTEGRATED
# RUN: yaml2obj %t/gc.yaml -o %t/gc.o
# RUN: ld.lld --gc-sections -e no_bpo %t/gc.o -o %t/gc-none
# RUN: llvm-readobj --sections %t/gc-none \
# RUN:   | FileCheck %s --check-prefix=GC-NONE \
# RUN:     --implicit-check-not=.MMIX.reg_contents
# RUN: ld.lld --gc-sections -e live_bpo %t/gc.o -o %t/gc-one
# RUN: llvm-objdump -s --section=.text --section=.MMIX.reg_contents %t/gc-one \
# RUN:   | FileCheck %s --check-prefix=GC-ONE

# RELOCS:      Relocations [
# RELOCS-NEXT: ]
# INTEGRATED:      Contents of section .text:
# INTEGRATED-NEXT: {{[0-9a-f]+}} 2007fd00 feaaaaaa aaaaaaaa aaaaaaaa
# INTEGRATED:      Contents of section .MMIX.reg_contents:
# INTEGRATED-NEXT: 07e8 00000000 00001000 11223344 55667788

# GC-NONE: Name: .text
# GC-ONE:      Contents of section .text:
# GC-ONE-NEXT: {{[0-9a-f]+}} 23aafe00
# GC-ONE:      Contents of section .MMIX.reg_contents:
# GC-ONE-NEXT: 07f0 00000000 00000000

#--- use.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .text
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content: AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - { Offset: 0, Type: R_MMIX_REG, Symbol: direct }
      - { Offset: 1, Type: R_MMIX_REG_OR_BYTE, Symbol: immediate }
      - { Offset: 2, Type: R_MMIX_BASE_PLUS_OFFSET, Symbol: address }
      - { Offset: 4, Type: R_MMIX_REG, Symbol: content_register }
      - { Offset: 5, Type: R_MMIX_LOCAL, Symbol: local_limit }
Symbols:
  - { Name: direct, Index: 0xFF00, Value: 32 }
  - { Name: immediate, Index: SHN_ABS, Value: 7 }
  - { Name: address, Index: SHN_ABS, Value: 4096 }
  - { Name: local_limit, Index: SHN_ABS, Value: 252 }
  - { Name: content_register, Index: SHN_UNDEF, Binding: STB_GLOBAL }

#--- definitions.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .MMIX.reg_contents
    Type: SHT_PROGBITS
    AddressAlign: 8
    Content: 1122334455667788
Symbols:
  - { Name: content_register, Section: .MMIX.reg_contents, Binding: STB_GLOBAL }

#--- gc.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .text.no_bpo
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content: FD000000
  - Name: .text.live_bpo
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content: 23AA0000
  - Name: .text.dead_bpo
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content: 23BB0000
  - Name: .rela.text.live_bpo
    Type: SHT_RELA
    Link: .symtab
    Info: .text.live_bpo
    Relocations:
      - { Offset: 2, Type: R_MMIX_BASE_PLUS_OFFSET, Symbol: low }
  - Name: .rela.text.dead_bpo
    Type: SHT_RELA
    Link: .symtab
    Info: .text.dead_bpo
    Relocations:
      - { Offset: 2, Type: R_MMIX_BASE_PLUS_OFFSET, Symbol: high }
Symbols:
  - { Name: no_bpo, Section: .text.no_bpo, Binding: STB_GLOBAL }
  - { Name: live_bpo, Section: .text.live_bpo, Binding: STB_GLOBAL }
  - { Name: dead_bpo, Section: .text.dead_bpo, Binding: STB_GLOBAL }
  - { Name: low, Index: SHN_ABS, Value: 0, Binding: STB_GLOBAL }
  - { Name: high, Index: SHN_ABS, Value: 256, Binding: STB_GLOBAL }
