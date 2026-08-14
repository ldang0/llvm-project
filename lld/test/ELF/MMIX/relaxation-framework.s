# REQUIRES: mmix

# RUN: yaml2obj --docnum=1 %s -o %t-valid.o
# RUN: not ld.lld --verbose --error-limit=0 -e target %t-valid.o \
# RUN:   -o /dev/null 2>&1 | FileCheck %s --check-prefix=VALID
# RUN: yaml2obj --docnum=2 %s -o %t-invalid.o
# RUN: not ld.lld --verbose --error-limit=0 -e target %t-invalid.o \
# RUN:   -o /dev/null 2>&1 | FileCheck %s --check-prefix=INVALID

## All primary expanding relocations enter one target-local relaxation pass.
## Family-specific rewrites are introduced by subsequent tasks, so valid
## reservations still end in the established stable diagnostic.
# VALID-DAG: MMIX relaxation passes: 1
# VALID-DAG: unsupported relocation R_MMIX_GETA against symbol target: requires MMIX relaxation support
# VALID-DAG: unsupported relocation R_MMIX_CBRANCH against symbol target: requires MMIX relaxation support
# VALID-DAG: unsupported relocation R_MMIX_PUSHJ against symbol target: requires MMIX relaxation support
# VALID-DAG: unsupported relocation R_MMIX_JMP against symbol target: requires MMIX relaxation support

## Reject malformed reservations before a relaxation implementation can
## inspect or rewrite their instructions.
# INVALID-DAG: relaxation relocation R_MMIX_GETA offset 2 is not 4-byte aligned against symbol target
# INVALID-DAG: R_MMIX_GETA reserved sequence has an invalid primary instruction at offset 0 against symbol target
# INVALID-DAG: R_MMIX_GETA reserved sequence contains non-SWYM padding at offset 0 against symbol target
# INVALID-DAG: R_MMIX_GETA requires a 16-byte reserved sequence at offset 0 against symbol target
# INVALID-DAG: R_MMIX_GETA requires a 16-byte reserved sequence at offset 8 against symbol target
# INVALID-DAG: unsupported relocation R_MMIX_GETA_1 against symbol target: GNU relaxation continuation cannot be used as standalone input

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
    Content:      F4010000FD000000FD000000FD00000040010000FD000000FD000000FD000000FD000000FD000000F2010000FD000000FD000000FD000000FD000000F0000000FD000000FD000000FD000000FD000000FD000000
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - { Offset: 0,  Type: R_MMIX_GETA,    Symbol: target, Addend: 0 }
      - { Offset: 16, Type: R_MMIX_CBRANCH, Symbol: target, Addend: 0 }
      - { Offset: 40, Type: R_MMIX_PUSHJ,   Symbol: target, Addend: 0 }
      - { Offset: 60, Type: R_MMIX_JMP,     Symbol: target, Addend: 0 }
Symbols:
  - Name:    target
    Section: .text
    Value:   80
    Binding: STB_GLOBAL

--- !ELF
FileHeader:
  Class:   ELFCLASS64
  Data:    ELFDATA2MSB
  Type:    ET_REL
  Machine: EM_MMIX
Sections:
  - Name:         .unaligned
    Type:         SHT_PROGBITS
    Flags:        [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 1
    Content:      0000F4010000FD000000FD000000FD0000000000
  - Name: .rela.unaligned
    Type: SHT_RELA
    Link: .symtab
    Info: .unaligned
    Relocations:
      - { Offset: 2, Type: R_MMIX_GETA, Symbol: target, Addend: 0 }
  - Name:         .bad_opcode
    Type:         SHT_PROGBITS
    Flags:        [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content:      FD000000FD000000FD000000FD000000
  - Name: .rela.bad_opcode
    Type: SHT_RELA
    Link: .symtab
    Info: .bad_opcode
    Relocations:
      - { Offset: 0, Type: R_MMIX_GETA, Symbol: target, Addend: 0 }
  - Name:         .bad_padding
    Type:         SHT_PROGBITS
    Flags:        [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content:      F401000000000000FD000000FD000000
  - Name: .rela.bad_padding
    Type: SHT_RELA
    Link: .symtab
    Info: .bad_padding
    Relocations:
      - { Offset: 0, Type: R_MMIX_GETA, Symbol: target, Addend: 0 }
  - Name:         .truncated
    Type:         SHT_PROGBITS
    Flags:        [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content:      F4010000FD000000FD000000
  - Name: .rela.truncated
    Type: SHT_RELA
    Link: .symtab
    Info: .truncated
    Relocations:
      - { Offset: 0, Type: R_MMIX_GETA, Symbol: target, Addend: 0 }
  - Name:         .outside
    Type:         SHT_PROGBITS
    Flags:        [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content:      FD000000
  - Name: .rela.outside
    Type: SHT_RELA
    Link: .symtab
    Info: .outside
    Relocations:
      - { Offset: 8, Type: R_MMIX_GETA, Symbol: target, Addend: 0 }
  - Name:         .continuation
    Type:         SHT_PROGBITS
    Flags:        [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content:      FD000000
  - Name: .rela.continuation
    Type: SHT_RELA
    Link: .symtab
    Info: .continuation
    Relocations:
      - { Offset: 0, Type: R_MMIX_GETA_1, Symbol: target, Addend: 0 }
  - Name:         .target
    Type:         SHT_PROGBITS
    Flags:        [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content:      FD000000
Symbols:
  - Name:    target
    Section: .target
    Binding: STB_GLOBAL
