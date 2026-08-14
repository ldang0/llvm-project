# REQUIRES: mmix

# RUN: rm -rf %t && split-file %s %t
# RUN: yaml2obj %t/default.yaml -o %t/default.o
# RUN: ld.lld -e 0 %t/default.o -o %t/default
# RUN: llvm-readobj --relocations %t/default | FileCheck %s --check-prefix=RELOCS
# RUN: llvm-objdump -s --section=.text %t/default \
# RUN:   | FileCheck %s --check-prefix=UNCHANGED
# RUN: yaml2obj %t/input-pass.yaml -o %t/input-pass.o
# RUN: ld.lld -e 0 %t/input-pass.o -o %t/input-pass
# RUN: yaml2obj %t/input-fail.yaml -o %t/input-fail.o
# RUN: not ld.lld -e 0 %t/input-fail.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=INPUT-BOUNDARY
# RUN: yaml2obj %t/allocated-pass.yaml -o %t/allocated-pass.o
# RUN: ld.lld -e 0 %t/allocated-pass.o -o %t/allocated-pass
# RUN: yaml2obj %t/allocated-fail.yaml -o %t/allocated-fail.o
# RUN: not ld.lld -e 0 %t/allocated-fail.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=ALLOCATED-BOUNDARY
# RUN: yaml2obj %t/invalid.yaml -o %t/invalid.o
# RUN: not ld.lld --error-limit=0 -e 0 %t/invalid.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=INVALID
# RUN: yaml2obj %t/invalid-offset.yaml -o %t/invalid-offset.o
# RUN: not ld.lld -e 0 %t/invalid-offset.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=OFFSET

# RELOCS:      Relocations [
# RELOCS-NEXT: ]
# UNCHANGED:      Contents of section .text:
# UNCHANGED-NEXT: {{[0-9a-f]+}} aabbccdd

# INPUT-BOUNDARY: R_MMIX_LOCAL register $254 is not local; first global register is $254
# ALLOCATED-BOUNDARY: R_MMIX_LOCAL register $254 is not local; first global register is $254
# INVALID-DAG: relocation R_MMIX_LOCAL requires a register or absolute value, but ordinary is neither
# INVALID-DAG: relocation R_MMIX_LOCAL requires a register or absolute value, but common is neither
# INVALID-DAG: register-content symbol content_register with addend is not 8-byte aligned
# INVALID-DAG: relocation R_MMIX_LOCAL register calculation overflows the 64-bit address range
# INVALID-DAG: relocation R_MMIX_LOCAL resolves outside the register range [0, 255]
# OFFSET: R_MMIX_LOCAL metadata offset 4 is outside the section against symbol absolute

#--- default.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .text
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content: AABBCCDD
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - { Offset: 0, Type: R_MMIX_LOCAL, Symbol: absolute, Addend: 1 }
      - { Offset: 1, Type: R_MMIX_LOCAL, Symbol: direct, Addend: 1 }
Symbols:
  - { Name: direct, Index: 0xFF00, Value: 32 }
  - { Name: absolute, Index: SHN_ABS, Value: 253, Binding: STB_GLOBAL }

#--- input-pass.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .text
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    Content: FD000000
  - Name: .MMIX.reg_contents
    Type: SHT_PROGBITS
    AddressAlign: 8
    Content: 0000000000000000
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - { Offset: 0, Type: R_MMIX_LOCAL, Symbol: below }
Symbols:
  - { Name: below, Index: SHN_ABS, Value: 253 }

#--- input-fail.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .text
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    Content: FD000000
  - Name: .MMIX.reg_contents
    Type: SHT_PROGBITS
    AddressAlign: 8
    Content: 0000000000000000
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - { Offset: 0, Type: R_MMIX_LOCAL, Symbol: boundary }
Symbols:
  - { Name: boundary, Index: SHN_ABS, Value: 254 }

#--- allocated-pass.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .text
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    Content: 23AA0000FD000000
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - { Offset: 2, Type: R_MMIX_BASE_PLUS_OFFSET, Symbol: address }
      - { Offset: 4, Type: R_MMIX_LOCAL, Symbol: below }
Symbols:
  - { Name: address, Index: SHN_ABS, Value: 4096 }
  - { Name: below, Index: SHN_ABS, Value: 253 }

#--- allocated-fail.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .text
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    Content: 23AA0000FD000000
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - { Offset: 2, Type: R_MMIX_BASE_PLUS_OFFSET, Symbol: address }
      - { Offset: 4, Type: R_MMIX_LOCAL, Symbol: boundary }
Symbols:
  - { Name: address, Index: SHN_ABS, Value: 4096 }
  - { Name: boundary, Index: SHN_ABS, Value: 254 }

#--- invalid.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .text
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    Content: FDFDFDFDFDFD0000
  - Name: .data
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_WRITE ]
    Content: 00
  - Name: .MMIX.reg_contents
    Type: SHT_PROGBITS
    AddressAlign: 8
    Content: 0000000000000000
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - { Offset: 0, Type: R_MMIX_LOCAL, Symbol: ordinary }
      - { Offset: 1, Type: R_MMIX_LOCAL, Symbol: common }
      - { Offset: 2, Type: R_MMIX_LOCAL, Symbol: content_register, Addend: 1 }
      - { Offset: 3, Type: R_MMIX_LOCAL, Symbol: direct, Addend: -33 }
      - { Offset: 4, Type: R_MMIX_LOCAL, Symbol: too_large }
Symbols:
  - { Name: direct, Index: 0xFF00, Value: 32 }
  - { Name: ordinary, Section: .data, Binding: STB_GLOBAL }
  - { Name: common, Index: SHN_COMMON, Value: 8, Size: 8, Binding: STB_GLOBAL }
  - { Name: content_register, Section: .MMIX.reg_contents, Binding: STB_GLOBAL }
  - { Name: too_large, Index: SHN_ABS, Value: 256, Binding: STB_GLOBAL }

#--- invalid-offset.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .text
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    Content: FD000000
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - { Offset: 4, Type: R_MMIX_LOCAL, Symbol: absolute }
Symbols:
  - { Name: absolute, Index: SHN_ABS, Value: 0 }
