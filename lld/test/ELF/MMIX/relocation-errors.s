# REQUIRES: mmix

# RUN: rm -rf %t && split-file %s %t
# RUN: yaml2obj %t/fields.yaml -o %t/fields.o
# RUN: not ld.lld -e 0 %t/fields.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=FIELDS
# RUN: yaml2obj %t/alignment.yaml -o %t/alignment.o
# RUN: not ld.lld -e 0 %t/alignment.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=ALIGNMENT
# RUN: yaml2obj %t/undefined.yaml -o %t/undefined.o
# RUN: not ld.lld -e 0 %t/undefined.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=UNDEFINED
# RUN: yaml2obj %t/weak.yaml -o %t/weak.o
# RUN: ld.lld -e 0 -T %t/layout.lds %t/weak.o -o %t/weak
# RUN: llvm-objdump -s --section=.data %t/weak \
# RUN:   | FileCheck %s --check-prefix=WEAK
# RUN: yaml2obj %t/invalid-symbol.yaml -o %t/invalid-symbol.o
# RUN: env LLD_IN_TEST=1 not ld.lld -e 0 %t/invalid-symbol.o \
# RUN:   -o /dev/null 2>&1 | FileCheck %s --check-prefix=INVALID-SYMBOL
# RUN: yaml2obj %t/tls.yaml -o %t/tls.o
# RUN: not ld.lld -e 0 %t/tls.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=TLS

## The absolute and PC-relative relocation tests own exact accepted and first
## rejected values for every data width. The ADDR19 and ADDR27 tests own their
## corresponding control boundaries and target-alignment diagnostics.

# FIELDS-DAG: fields.o:(.data): relocation R_MMIX_PC_8 offset 8 is outside the section
# FIELDS-DAG: fields.o:(.data): relocation R_MMIX_PC_16 field at offset 7 extends past the end of the section
# FIELDS-DAG: fields.o:(.data): relocation R_MMIX_PC_24 field at offset 5 extends past the end of the section
# FIELDS-DAG: fields.o:(.data): relocation R_MMIX_PC_32 field at offset 5 extends past the end of the section
# FIELDS-DAG: fields.o:(.data): relocation R_MMIX_PC_64 field at offset 1 extends past the end of the section
# FIELDS-DAG: fields.o:(.text): relocation R_MMIX_ADDR19 field at offset 5 extends past the end of the section
# FIELDS-DAG: fields.o:(.text): relocation R_MMIX_ADDR27 offset 8 is outside the section

# ALIGNMENT-DAG: alignment.o:(.text): relocation R_MMIX_ADDR19 field offset 1 is not 4-byte aligned
# ALIGNMENT-DAG: alignment.o:(.text): relocation R_MMIX_ADDR27 field offset 5 is not 4-byte aligned

# UNDEFINED: undefined symbol: missing
# UNDEFINED: referenced by {{.*}}undefined.o:(.data+0x0)

## Undefined weak symbols resolve to zero in a static link. The absolute field
## therefore receives A, while the PC-relative field receives A-P.
# WEAK:      Contents of section .data:
# WEAK-NEXT: 5000 00000000 00000123 00000000 00000010

# INVALID-SYMBOL: invalid-symbol.o: invalid symbol index
# TLS: tls.o:(.data+0x0): relocation R_MMIX_64 against TLS symbol tls is unsupported

#--- layout.lds
SECTIONS { .data 0x5000 : { *(.data) } }

#--- fields.yaml
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
    Content:      0000000000000000
  - Name:         .data
    Type:         SHT_PROGBITS
    Flags:        [ SHF_ALLOC, SHF_WRITE ]
    AddressAlign: 8
    Content:      0000000000000000
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - { Offset: 5, Type: R_MMIX_ADDR19, Symbol: zero, Addend: 0 }
      - { Offset: 8, Type: R_MMIX_ADDR27, Symbol: zero, Addend: 0 }
  - Name: .rela.data
    Type: SHT_RELA
    Link: .symtab
    Info: .data
    Relocations:
      - { Offset: 8, Type: R_MMIX_PC_8, Symbol: zero, Addend: 0 }
      - { Offset: 7, Type: R_MMIX_PC_16, Symbol: zero, Addend: 0 }
      - { Offset: 5, Type: R_MMIX_PC_24, Symbol: zero, Addend: 0 }
      - { Offset: 5, Type: R_MMIX_PC_32, Symbol: zero, Addend: 0 }
      - { Offset: 1, Type: R_MMIX_PC_64, Symbol: zero, Addend: 0 }
Symbols:
  - { Name: zero, Index: SHN_ABS, Value: 0 }

#--- alignment.yaml
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
    AddressAlign: 1
    Content:      000000000000000000000000
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - { Offset: 1, Type: R_MMIX_ADDR19, Symbol: addr19, Addend: 0 }
      - { Offset: 5, Type: R_MMIX_ADDR27, Symbol: addr27, Addend: 0 }
Symbols:
  - { Name: addr19, Index: SHN_ABS, Value: 5 }
  - { Name: addr27, Index: SHN_ABS, Value: 9 }

#--- undefined.yaml
--- !ELF
FileHeader:
  Class:   ELFCLASS64
  Data:    ELFDATA2MSB
  Type:    ET_REL
  Machine: EM_MMIX
Sections:
  - Name:         .data
    Type:         SHT_PROGBITS
    Flags:        [ SHF_ALLOC, SHF_WRITE ]
    AddressAlign: 8
    Content:      0000000000000000
  - Name: .rela.data
    Type: SHT_RELA
    Link: .symtab
    Info: .data
    Relocations:
      - { Offset: 0, Type: R_MMIX_64, Symbol: missing, Addend: 0 }
Symbols:
  - { Name: missing, Binding: STB_GLOBAL }

#--- weak.yaml
--- !ELF
FileHeader:
  Class:   ELFCLASS64
  Data:    ELFDATA2MSB
  Type:    ET_REL
  Machine: EM_MMIX
Sections:
  - Name:         .data
    Type:         SHT_PROGBITS
    Flags:        [ SHF_ALLOC, SHF_WRITE ]
    AddressAlign: 8
    Content:      00000000000000000000000000000000
  - Name: .rela.data
    Type: SHT_RELA
    Link: .symtab
    Info: .data
    Relocations:
      - { Offset: 0, Type: R_MMIX_64, Symbol: weak, Addend: 0x123 }
      - { Offset: 8, Type: R_MMIX_PC_64, Symbol: weak, Addend: 0x5018 }
Symbols:
  - { Name: weak, Binding: STB_WEAK }

#--- invalid-symbol.yaml
--- !ELF
FileHeader:
  Class:   ELFCLASS64
  Data:    ELFDATA2MSB
  Type:    ET_REL
  Machine: EM_MMIX
Sections:
  - Name:         .data
    Type:         SHT_PROGBITS
    Flags:        [ SHF_ALLOC, SHF_WRITE ]
    AddressAlign: 8
    Content:      0000000000000000
  - Name: .rela.data
    Type: SHT_RELA
    Link: .symtab
    Info: .data
    Relocations:
      - { Offset: 0, Type: R_MMIX_64, Symbol: 255, Addend: 0 }
Symbols:
  - { Name: dummy, Index: SHN_ABS, Value: 0 }

#--- tls.yaml
--- !ELF
FileHeader:
  Class:   ELFCLASS64
  Data:    ELFDATA2MSB
  Type:    ET_REL
  Machine: EM_MMIX
Sections:
  - Name:         .data
    Type:         SHT_PROGBITS
    Flags:        [ SHF_ALLOC, SHF_WRITE ]
    AddressAlign: 8
    Content:      0000000000000000
  - Name:         .tdata
    Type:         SHT_PROGBITS
    Flags:        [ SHF_ALLOC, SHF_WRITE, SHF_TLS ]
    AddressAlign: 8
    Content:      0000000000000000
  - Name: .rela.data
    Type: SHT_RELA
    Link: .symtab
    Info: .data
    Relocations:
      - { Offset: 0, Type: R_MMIX_64, Symbol: tls, Addend: 0 }
Symbols:
  - { Name: tls, Type: STT_TLS, Section: .tdata, Binding: STB_GLOBAL }
