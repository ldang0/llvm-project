# REQUIRES: mmix

# RUN: rm -rf %t && split-file %s %t
# RUN: yaml2obj %t/a.yaml -o %t/a.o
# RUN: yaml2obj %t/b.yaml -o %t/b.o
# RUN: ld.lld -e a -Map=%t/one.map %t/a.o %t/b.o -o %t/one
# RUN: ld.lld -e a -Map=%t/two.map %t/a.o %t/b.o -o %t/two
# RUN: cmp %t/one %t/two
# RUN: cmp %t/one.map %t/two.map
# RUN: ld.lld -e a %t/b.o %t/a.o -o %t/reordered
# RUN: llvm-readobj --symbols --relocations %t/one \
# RUN:   | FileCheck %s --check-prefix=SEMANTIC
# RUN: llvm-readobj --symbols --relocations %t/reordered \
# RUN:   | FileCheck %s --check-prefix=SEMANTIC
# RUN: llvm-objdump -s --section=.MMIX.reg_contents %t/one \
# RUN:   | FileCheck %s --check-prefix=REGS
# RUN: llvm-objdump -s --section=.MMIX.reg_contents %t/reordered \
# RUN:   | FileCheck %s --check-prefix=REGS

# SEMANTIC:      Relocations [
# SEMANTIC-NEXT: ]
# SEMANTIC-DAG:  Name: __MMIX_call_stub_0
# SEMANTIC-DAG:  Name: a
# SEMANTIC-DAG:  Name: b
# REGS:          Contents of section .MMIX.reg_contents:
# REGS-NEXT:     {{[0-9a-f]+}} 00000000 00000000 00000000 00000100

#--- a.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .text.a
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content: F200000023AA0000
  - Name: .rela.text.a
    Type: SHT_RELA
    Link: .symtab
    Info: .text.a
    Relocations:
      - { Offset: 0, Type: R_MMIX_PUSHJ_STUBBABLE, Symbol: far_target }
      - { Offset: 6, Type: R_MMIX_BASE_PLUS_OFFSET, Symbol: low }
Symbols:
  - { Name: a, Section: .text.a, Binding: STB_GLOBAL }
  - { Name: far_target, Index: SHN_ABS, Value: 4294967296, Binding: STB_GLOBAL }
  - { Name: low, Index: SHN_ABS, Value: 0, Binding: STB_GLOBAL }

#--- b.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .text.b
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content: 23BB0000
  - Name: .rela.text.b
    Type: SHT_RELA
    Link: .symtab
    Info: .text.b
    Relocations:
      - { Offset: 2, Type: R_MMIX_BASE_PLUS_OFFSET, Symbol: high }
Symbols:
  - { Name: b, Section: .text.b, Binding: STB_GLOBAL }
  - { Name: high, Index: SHN_ABS, Value: 256, Binding: STB_GLOBAL }
