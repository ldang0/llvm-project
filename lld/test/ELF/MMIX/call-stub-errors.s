# REQUIRES: mmix

# RUN: rm -rf %t && split-file %s %t
# RUN: yaml2obj %t/malformed.yaml -o %t/malformed.o
# RUN: not ld.lld --error-limit=0 -e 0 %t/malformed.o -o /dev/null 2>&1 | FileCheck %s --check-prefix=MALFORMED
# RUN: llvm-mc -triple=mmix -filetype=obj %t/unaligned.s -o %t/unaligned.o
# RUN: not ld.lld -e _start %t/unaligned.o -o /dev/null 2>&1 | FileCheck %s --check-prefix=UNALIGNED
# RUN: llvm-mc -triple=mmix -filetype=obj %t/unreachable.s -o %t/unreachable.o
# RUN: not ld.lld -T %t/layout.lds %t/unreachable.o -o /dev/null 2>&1 | FileCheck %s --check-prefix=UNREACHABLE
# RUN: llvm-mc -triple=mmix -filetype=obj %t/undefined.s -o %t/undefined.o
# RUN: not ld.lld -e _start %t/undefined.o -o /dev/null 2>&1 | FileCheck %s --check-prefix=UNDEFINED

# MALFORMED: R_MMIX_PUSHJ_STUBBABLE does not reference a PUSHJ instruction at offset 0 against symbol target
# MALFORMED-DAG: R_MMIX_PUSHJ_STUBBABLE requires a 4-byte PUSHJ instruction at offset 0 against symbol truncated
# MALFORMED-DAG: relaxation relocation R_MMIX_PUSHJ_STUBBABLE is not in an executable section against symbol data_target
# UNALIGNED: relocation R_MMIX_PUSHJ_STUBBABLE against symbol has a target that is not 4-byte aligned
# UNREACHABLE: relocation R_MMIX_PUSHJ_STUBBABLE against symbol cannot reach its section-end stub
# UNDEFINED: undefined symbol: missing

#--- malformed.yaml
--- !ELF
FileHeader:
  Class: ELFCLASS64
  Data: ELFDATA2MSB
  Type: ET_REL
  Machine: EM_MMIX
Sections:
  - Name: .text
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content: FD000000
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - { Offset: 0, Type: R_MMIX_PUSHJ_STUBBABLE, Symbol: target }
  - Name: .truncated
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
  - Name: .rela.truncated
    Type: SHT_RELA
    Link: .symtab
    Info: .truncated
    Relocations:
      - { Offset: 0, Type: R_MMIX_PUSHJ_STUBBABLE, Symbol: truncated }
  - Name: .data
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_WRITE ]
    AddressAlign: 4
    Content: F2010000
  - Name: .rela.data
    Type: SHT_RELA
    Link: .symtab
    Info: .data
    Relocations:
      - { Offset: 0, Type: R_MMIX_PUSHJ_STUBBABLE, Symbol: data_target }
Symbols:
  - Name: target
    Section: .text
  - Name: truncated
    Section: .truncated
  - Name: data_target
    Section: .data

#--- unaligned.s
.text
.global _start
_start:
PUSHJ r1, target
.global target
.set target, 3

#--- unreachable.s
.text
.global _start
_start:
PUSHJ r1, far_target
.space 0x40000
.global far_target
.set far_target, 0x1122334455667788

#--- undefined.s
.text
.global _start
_start:
PUSHJ r1, missing
.global missing

#--- layout.lds
ENTRY(_start)
SECTIONS { .text 0x1000 : { *(.text) } }
