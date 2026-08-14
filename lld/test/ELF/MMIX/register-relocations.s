# REQUIRES: mmix

# RUN: rm -rf %t && split-file %s %t
# RUN: yaml2obj %t/use.yaml -o %t/use.o
# RUN: yaml2obj %t/definitions.yaml -o %t/definitions.o
# RUN: ld.lld -e _start %t/use.o %t/definitions.o -o %t/executable
# RUN: llvm-readobj --relocations %t/executable | FileCheck %s --check-prefix=RELOCS
# RUN: llvm-objdump -s --section=.text %t/executable | FileCheck %s --check-prefix=CONTENTS
# RUN: yaml2obj %t/invalid.yaml -o %t/invalid.o
# RUN: not ld.lld --error-limit=0 -e 0 %t/invalid.o -o /dev/null 2>&1 | FileCheck %s --check-prefix=INVALID
# RUN: yaml2obj %t/invalid-offset.yaml -o %t/invalid-offset.o
# RUN: not ld.lld -e 0 %t/invalid-offset.o -o /dev/null 2>&1 | FileCheck %s --check-prefix=INVALID-OFFSET

# RELOCS:      Relocations [
# RELOCS-NEXT: ]
# CONTENTS:      Contents of section .text:
# CONTENTS-NEXT: {{[0-9a-f]+}} aa21fffe 00ffffbb

# INVALID-DAG: relocation R_MMIX_REG requires a register symbol, but absolute is not one
# INVALID-DAG: relocation R_MMIX_REG requires a register symbol, but ordinary is not one
# INVALID-DAG: relocation R_MMIX_REG_OR_BYTE requires a register symbol or absolute byte, but ordinary is neither
# INVALID-DAG: relocation R_MMIX_REG_OR_BYTE resolves to 256, outside byte range [0, 255]
# INVALID-DAG: register-content symbol content_register with addend is not 8-byte aligned
# INVALID-DAG: register-content symbol content_register with addend resolves outside .MMIX.reg_contents
# INVALID-DAG: relocation R_MMIX_REG resolves to 256, outside byte range [0, 255]
# INVALID-OFFSET: relocation R_MMIX_REG offset 4 is outside the section against symbol valid_register

#--- use.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .text
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content: AAAAAAAAAAAAAABB
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - { Offset: 1, Type: R_MMIX_REG, Symbol: local_register, Addend: 1 }
      - { Offset: 2, Type: R_MMIX_REG, Symbol: global_register, Addend: 0 }
      - { Offset: 3, Type: R_MMIX_REG, Symbol: content_register, Addend: 0 }
      - { Offset: 4, Type: R_MMIX_REG_OR_BYTE, Symbol: byte_zero, Addend: 0 }
      - { Offset: 5, Type: R_MMIX_REG_OR_BYTE, Symbol: byte_limit, Addend: 1 }
      - { Offset: 6, Type: R_MMIX_REG_OR_BYTE, Symbol: local_register, Addend: 223 }
Symbols:
  - { Name: local_register, Index: 0xFF00, Value: 32 }
  - { Name: _start, Section: .text, Binding: STB_GLOBAL, Type: STT_FUNC }
  - { Name: global_register, Index: SHN_UNDEF, Binding: STB_GLOBAL }
  - { Name: content_register, Index: SHN_UNDEF, Binding: STB_GLOBAL }
  - { Name: byte_zero, Index: SHN_ABS, Value: 0, Binding: STB_GLOBAL }
  - { Name: byte_limit, Index: SHN_ABS, Value: 254, Binding: STB_GLOBAL }

#--- definitions.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .MMIX.reg_contents
    Type: SHT_PROGBITS
    AddressAlign: 8
    Content: 1122334455667788
Symbols:
  - { Name: global_register, Index: 0xFF00, Value: 255, Binding: STB_GLOBAL }
  - { Name: content_register, Section: .MMIX.reg_contents, Binding: STB_GLOBAL }

#--- invalid.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .text
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content: AAAAAAAAAAAAAAAA
  - Name: .MMIX.reg_contents
    Type: SHT_PROGBITS
    AddressAlign: 8
    Content: 0000000000000000
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - { Offset: 0, Type: R_MMIX_REG, Symbol: absolute }
      - { Offset: 1, Type: R_MMIX_REG, Symbol: ordinary }
      - { Offset: 2, Type: R_MMIX_REG_OR_BYTE, Symbol: ordinary }
      - { Offset: 3, Type: R_MMIX_REG_OR_BYTE, Symbol: absolute, Addend: 256 }
      - { Offset: 4, Type: R_MMIX_REG, Symbol: content_register, Addend: 1 }
      - { Offset: 5, Type: R_MMIX_REG, Symbol: content_register, Addend: 8 }
      - { Offset: 6, Type: R_MMIX_REG, Symbol: valid_register, Addend: 224 }
Symbols:
  - { Name: valid_register, Index: 0xFF00, Value: 32 }
  - { Name: absolute, Index: SHN_ABS, Binding: STB_GLOBAL }
  - { Name: ordinary, Section: .text, Binding: STB_GLOBAL }
  - { Name: content_register, Section: .MMIX.reg_contents, Binding: STB_GLOBAL }

#--- invalid-offset.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .text
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content: AAAAAAAA
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - { Offset: 4, Type: R_MMIX_REG, Symbol: valid_register }
Symbols:
  - { Name: valid_register, Index: 0xFF00, Value: 32 }
