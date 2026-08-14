# REQUIRES: mmix

# RUN: rm -rf %t && split-file %s %t
# RUN: yaml2obj %t/bpo-overflow.yaml -o %t/bpo-overflow.o
# RUN: not ld.lld -e 0 %t/bpo-overflow.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=BPO-OVERFLOW
# RUN: yaml2obj %t/reg-overflow.yaml -o %t/reg-overflow.o
# RUN: not ld.lld -e 0 %t/reg-overflow.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=REG-OVERFLOW
# RUN: yaml2obj %t/conflict-a.yaml -o %t/conflict-a.o
# RUN: yaml2obj %t/conflict-b.yaml -o %t/conflict-b.o
# RUN: not ld.lld -e 0 %t/conflict-a.o %t/conflict-b.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=CONFLICT

# BPO-OVERFLOW: relocation R_MMIX_BASE_PLUS_OFFSET address calculation overflows the 64-bit address range
# REG-OVERFLOW: relocation R_MMIX_REG register calculation overflows the 64-bit address range
# CONFLICT: duplicate symbol: shared_register

#--- bpo-overflow.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .text
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content: 23AA0000
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - { Offset: 2, Type: R_MMIX_BASE_PLUS_OFFSET, Symbol: maximum, Addend: 1 }
Symbols:
  - { Name: maximum, Index: SHN_ABS, Value: 0xFFFFFFFFFFFFFFFF }

#--- reg-overflow.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .text
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    Content: AAAAAAAA
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - { Offset: 0, Type: R_MMIX_REG, Symbol: direct, Addend: -33 }
Symbols:
  - { Name: direct, Index: 0xFF00, Value: 32 }

#--- conflict-a.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .text
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    Content: FD000000
Symbols:
  - { Name: shared_register, Index: 0xFF00, Value: 32, Binding: STB_GLOBAL }

#--- conflict-b.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Symbols:
  - { Name: shared_register, Index: 0xFF00, Value: 33, Binding: STB_GLOBAL }
