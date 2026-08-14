# REQUIRES: mmix

# RUN: rm -rf %t && split-file %s %t
# RUN: yaml2obj %t/valid.yaml -o %t/valid.o
# RUN: ld.lld -e 0 -T %t/layout.lds %t/valid.o -o %t/valid
# RUN: llvm-objdump -s --section=.text %t/valid \
# RUN:   | FileCheck %s --check-prefix=VALID
# RUN: llvm-readobj --sections %t/valid | FileCheck %s --check-prefix=SIZE
# RUN: yaml2obj %t/invalid.yaml -o %t/invalid.o
# RUN: not ld.lld -e 0 -T %t/layout.lds %t/invalid.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=INVALID

## R_MMIX_ADDR19 writes D=(S+A-P)/4 into the low 16 bits and derives bit 24
## from the sign of D. The remaining instruction bits come from the input.
# VALID:      Contents of section .text:
# VALID-NEXT: 4000 aacd0001 55efffff f0aa0000 13000000
# VALID-NEXT: 4010 fe11ffff 80cc0010

# SIZE-LABEL: Name: .text
# SIZE:       Size: 24

# INVALID-DAG: improper alignment for relocation R_MMIX_ADDR19: 0x2 is not aligned to 4 bytes
# INVALID-DAG: relocation R_MMIX_ADDR19 out of range: -262148 is not in [-262144, 262140]
# INVALID-DAG: relocation R_MMIX_ADDR19 out of range: 262144 is not in [-262144, 262140]

#--- layout.lds
SECTIONS { .text 0x4000 : { *(.text) } }

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
    Content:      ABCD123454EF5678F1AABCDE1200FFFFFF11AAAA80CC5555
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - { Offset: 0,  Type: R_MMIX_ADDR19, Symbol: forward, Addend: 0 }
      - { Offset: 4,  Type: R_MMIX_ADDR19, Symbol: backward, Addend: 0 }
      - { Offset: 8,  Type: R_MMIX_ADDR19, Symbol: zero, Addend: 0 }
      - { Offset: 12, Type: R_MMIX_ADDR19, Symbol: lower, Addend: 0 }
      - { Offset: 16, Type: R_MMIX_ADDR19, Symbol: upper, Addend: 0 }
      - { Offset: 20, Type: R_MMIX_ADDR19, Symbol: addend, Addend: 32 }
Symbols:
  - { Name: forward,  Index: SHN_ABS, Value: 0x4004 }
  - { Name: backward, Index: SHN_ABS, Value: 0x4000 }
  - { Name: zero,     Index: SHN_ABS, Value: 0x4008 }
  - { Name: lower,    Index: SHN_ABS, Value: 0xfffffffffffc400c }
  - { Name: upper,    Index: SHN_ABS, Value: 0x4400c }
  - { Name: addend,   Index: SHN_ABS, Value: 0x4034 }

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
    Content:      000000000000000000000000
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - { Offset: 0, Type: R_MMIX_ADDR19, Symbol: unaligned, Addend: 0 }
      - { Offset: 4, Type: R_MMIX_ADDR19, Symbol: below, Addend: 0 }
      - { Offset: 8, Type: R_MMIX_ADDR19, Symbol: above, Addend: 0 }
Symbols:
  - { Name: unaligned, Index: SHN_ABS, Value: 0x4002 }
  - { Name: below,     Index: SHN_ABS, Value: 0xfffffffffffc4000 }
  - { Name: above,     Index: SHN_ABS, Value: 0x44008 }
