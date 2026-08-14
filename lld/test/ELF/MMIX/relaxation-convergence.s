# REQUIRES: mmix

# RUN: rm -rf %t && split-file %s %t
# RUN: yaml2obj %t/a.yaml -o %t/a.o
# RUN: yaml2obj %t/b.yaml -o %t/b.o
# RUN: ld.lld --verbose -T %t/layout.lds %t/a.o %t/b.o -o %t/ab 2>&1 \
# RUN:   | FileCheck %s --check-prefix=PASSES
# RUN: ld.lld --verbose -T %t/layout.lds %t/b.o %t/a.o -o %t/ba 2>&1 \
# RUN:   | FileCheck %s --check-prefix=PASSES
# RUN: llvm-objdump -s --section=.text %t/ab | sed '1,2d' > %t/ab.txt
# RUN: llvm-objdump -s --section=.text %t/ba | sed '1,2d' > %t/ba.txt
# RUN: cmp %t/ab.txt %t/ba.txt
# RUN: ld.lld -T %t/layout.lds %t/a.o %t/b.o -o %t/ab-repeat
# RUN: cmp %t/ab %t/ab-repeat
# RUN: llvm-readobj --symbols --relocations %t/ab \
# RUN:   | FileCheck %s --check-prefix=STRUCTURE
# RUN: llvm-objdump -s --section=.text %t/ab \
# RUN:   | FileCheck %s --check-prefix=CONTENT

## Fixed-size primary relaxation classifies once and then verifies the same
## decisions against final addresses. Explicit script ordering makes the
## result independent of command-line object order.
# PASSES: MMIX relaxation passes: 2
# STRUCTURE:      Relocations [
# STRUCTURE-NEXT: ]
# STRUCTURE:      Name: a_target
# STRUCTURE-NEXT: Value: 0x1000
# STRUCTURE:      Name: b_target
# STRUCTURE-NEXT: Value: 0x1028
# STRUCTURE:      Name: alias_target
# STRUCTURE-NEXT: Value: 0x102C
# CONTENT:      Contents of section .text:
# CONTENT-NEXT: 1000 f401000b fd000000 fd000000 fd000000
# CONTENT-NEXT: 1010 40010006 fd000000 fd000000 fd000000
# CONTENT-NEXT: 1020 fd000000 fd000000 f2020001 fd000000
# CONTENT-NEXT: 1030 fd000000 fd000000 fd000000 e3ff7788
# CONTENT-NEXT: 1040 e6ff5566 e5ff3344 e4ff1122 9fffff00

#--- layout.lds
ENTRY(a_target)
SECTIONS {
  .text 0x1000 : { *(.text.a) *(.text.b) }
  far_target = 0x1122334455667788;
  alias_target = b_target + 4;
}

#--- a.yaml
--- !ELF
FileHeader:
  Class:   ELFCLASS64
  Data:    ELFDATA2MSB
  Type:    ET_REL
  Machine: EM_MMIX
Sections:
  - Name:         .text.a
    Type:         SHT_PROGBITS
    Flags:        [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content:      F4010000FD000000FD000000FD00000040010000FD000000FD000000FD000000FD000000FD000000
  - Name: .rela.text.a
    Type: SHT_RELA
    Link: .symtab
    Info: .text.a
    Relocations:
      - { Offset: 0,  Type: R_MMIX_GETA, Symbol: alias_target, Addend: 0 }
      - { Offset: 16, Type: R_MMIX_CBRANCH, Symbol: b_target, Addend: 0 }
Symbols:
  - { Name: a_target, Section: .text.a, Binding: STB_GLOBAL }
  - { Name: b_target, Index: SHN_UNDEF, Binding: STB_GLOBAL }
  - { Name: alias_target, Index: SHN_UNDEF, Binding: STB_GLOBAL }

#--- b.yaml
--- !ELF
FileHeader:
  Class:   ELFCLASS64
  Data:    ELFDATA2MSB
  Type:    ET_REL
  Machine: EM_MMIX
Sections:
  - Name:         .text.b
    Type:         SHT_PROGBITS
    Flags:        [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content:      F2020000FD000000FD000000FD000000FD000000F0000000FD000000FD000000FD000000FD000000
  - Name: .rela.text.b
    Type: SHT_RELA
    Link: .symtab
    Info: .text.b
    Relocations:
      - { Offset: 0,  Type: R_MMIX_PUSHJ, Symbol: section_target, Addend: 4 }
      - { Offset: 20, Type: R_MMIX_JMP, Symbol: far_target, Addend: 0 }
Symbols:
  - { Name: section_target, Type: STT_SECTION, Section: .text.b }
  - { Name: b_target, Section: .text.b, Binding: STB_GLOBAL }
  - { Name: far_target, Index: SHN_UNDEF, Binding: STB_GLOBAL }
