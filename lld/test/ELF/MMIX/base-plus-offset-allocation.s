# REQUIRES: mmix

# RUN: rm -rf %t && split-file %s %t
# RUN: yaml2obj %t/a.yaml -o %t/a.o
# RUN: yaml2obj %t/b.yaml -o %t/b.o
# RUN: ld.lld -e 0 -T %t/layout.ld %t/a.o %t/b.o -o %t/ab
# RUN: ld.lld -e 0 -T %t/layout.ld %t/b.o %t/a.o -o %t/ba
# RUN: llvm-objdump -s --section=.text --section=.MMIX.reg_contents %t/ab \
# RUN:   | FileCheck %s
# RUN: llvm-objdump -s --section=.text --section=.MMIX.reg_contents %t/ba \
# RUN:   | FileCheck %s
# RUN: yaml2obj %t/convergence.yaml -o %t/convergence.o
# RUN: ld.lld -e 0 -T %t/convergence.ld %t/convergence.o -o %t/convergence
# RUN: llvm-objdump -s --section=.text --section=.MMIX.reg_contents \
# RUN:   %t/convergence | FileCheck %s --check-prefix=CONVERGENCE

## Sorted final values 0, 255, 256, and 257 require exactly two bases. The
## exact upper boundary shares the first base; the next byte starts a new one.
# CHECK:      Contents of section .text:
# CHECK-NEXT: {{[0-9a-f]+}} 23aafe00 23bbfe01 23ccfd00 23ddfdff
# CHECK:      Contents of section .MMIX.reg_contents:
# CHECK-NEXT: 07e8 00000000 00000000 00000000 00000100

## The full call stub grows .text by 20 bytes. This moves after_text from 252
## to 272, so the allocator must recompute and select a second base. The weak
## undefined symbol follows generic static resolution and contributes base 0.
# CONVERGENCE:      Contents of section .text:
# CONVERGENCE-NEXT: {{[0-9a-f]+}} 23aafd00 23bbfe00 f2000001
# CONVERGENCE:      Contents of section .MMIX.reg_contents:
# CONVERGENCE-NEXT: 07e8 00000000 00000000 00000000 00000110

#--- layout.ld
SECTIONS {
  .text : { *(.text.a) *(.text.b) }
  .MMIX.reg_contents : {
    *(.MMIX.reg_contents.linker_allocated)
    *(.MMIX.reg_contents)
  }
}

#--- a.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .text.a
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content: 23AA000023BB0000
  - Name: .rela.text.a
    Type: SHT_RELA
    Link: .symtab
    Info: .text.a
    Relocations:
      - { Offset: 2, Type: R_MMIX_BASE_PLUS_OFFSET, Symbol: high, Addend: 0 }
      - { Offset: 6, Type: R_MMIX_BASE_PLUS_OFFSET, Symbol: high, Addend: 1 }
Symbols:
  - { Name: high, Index: SHN_ABS, Value: 256 }

#--- b.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .text.b
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content: 23CC000023DD0000
  - Name: .rela.text.b
    Type: SHT_RELA
    Link: .symtab
    Info: .text.b
    Relocations:
      - { Offset: 2, Type: R_MMIX_BASE_PLUS_OFFSET, Symbol: low, Addend: 0 }
      - { Offset: 6, Type: R_MMIX_BASE_PLUS_OFFSET, Symbol: low, Addend: 255 }
Symbols:
  - { Name: low, Index: SHN_ABS, Value: 0 }

#--- convergence.ld
SECTIONS {
  .text : { *(.text.use) *(.text.call) *(.text.pad) }
  .after : { *(.after) }
  .MMIX.reg_contents : {
    *(.MMIX.reg_contents.linker_allocated)
    *(.MMIX.reg_contents)
  }
}

#--- convergence.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .text.use
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content: 23AA000023BB0000
  - Name: .text.call
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content: F2000000
  - Name: .text.pad
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 1
    Size: 240
  - Name: .after
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC ]
    AddressAlign: 1
    Content: 00
  - Name: .rela.text.use
    Type: SHT_RELA
    Link: .symtab
    Info: .text.use
    Relocations:
      - { Offset: 2, Type: R_MMIX_BASE_PLUS_OFFSET, Symbol: weak_address }
      - { Offset: 6, Type: R_MMIX_BASE_PLUS_OFFSET, Symbol: after_text }
  - Name: .rela.text.call
    Type: SHT_RELA
    Link: .symtab
    Info: .text.call
    Relocations:
      - { Offset: 0, Type: R_MMIX_PUSHJ_STUBBABLE, Symbol: far_target }
Symbols:
  - { Name: after_text, Section: .after, Binding: STB_GLOBAL }
  - { Name: weak_address, Index: SHN_UNDEF, Binding: STB_WEAK }
  - { Name: far_target, Index: SHN_ABS, Value: 4294967296, Binding: STB_GLOBAL }
