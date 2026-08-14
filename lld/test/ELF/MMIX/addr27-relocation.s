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

## R_MMIX_ADDR27 writes D=(S+A-P)/4 into the low 24 bits and derives bit 24
## from the sign of D. Bits 31-25 come from the input instruction.
# VALID:      Contents of section .text:
# VALID-NEXT: 8000 aa000400 55fffbff f0000000 13000000
# VALID-NEXT: 8010 feffffff 80000020

# SIZE-LABEL: Name: .text
# SIZE:       Size: 24

# INVALID-DAG: improper alignment for relocation R_MMIX_ADDR27: 0x2 is not aligned to 4 bytes
# INVALID-DAG: relocation R_MMIX_ADDR27 out of range: -67108868 is not in [-67108864, 67108860]
# INVALID-DAG: relocation R_MMIX_ADDR27 out of range: 67108864 is not in [-67108864, 67108860]

#--- layout.lds
SECTIONS {
  .backward 0x7000 : { *(.backward) }
  .text 0x8000 : { *(.text) }
  .forward 0x9000 : { *(.forward) }
}

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
    Content:      ABFFFFFF54ABCDEFF1ABCDEE1200FFFFFF11AAAA80CC5555
  - Name:         .forward
    Type:         SHT_PROGBITS
    Flags:        [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content:      FD000000
  - Name:         .backward
    Type:         SHT_PROGBITS
    Flags:        [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content:      FD000000
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - { Offset: 0,  Type: R_MMIX_ADDR27, Symbol: forward, Addend: 0 }
      - { Offset: 4,  Type: R_MMIX_ADDR27, Symbol: backward, Addend: 0 }
      - { Offset: 8,  Type: R_MMIX_ADDR27, Symbol: zero, Addend: 0 }
      - { Offset: 12, Type: R_MMIX_ADDR27, Symbol: lower, Addend: 0 }
      - { Offset: 16, Type: R_MMIX_ADDR27, Symbol: upper, Addend: 0 }
      - { Offset: 20, Type: R_MMIX_ADDR27, Symbol: addend, Addend: -128 }
Symbols:
  - { Name: forward,  Section: .forward }
  - { Name: backward, Section: .backward }
  - { Name: zero,     Index: SHN_ABS, Value: 0x8008 }
  - { Name: lower,    Index: SHN_ABS, Value: 0xfffffffffc00800c }
  - { Name: upper,    Index: SHN_ABS, Value: 0x400800c }
  - { Name: addend,   Index: SHN_ABS, Value: 0x8114 }

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
      - { Offset: 0, Type: R_MMIX_ADDR27, Symbol: unaligned, Addend: 0 }
      - { Offset: 4, Type: R_MMIX_ADDR27, Symbol: below, Addend: 0 }
      - { Offset: 8, Type: R_MMIX_ADDR27, Symbol: above, Addend: 0 }
Symbols:
  - { Name: unaligned, Index: SHN_ABS, Value: 0x8002 }
  - { Name: below,     Index: SHN_ABS, Value: 0xfffffffffc008000 }
  - { Name: above,     Index: SHN_ABS, Value: 0x4008008 }
