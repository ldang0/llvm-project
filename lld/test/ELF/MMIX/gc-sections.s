# REQUIRES: mmix

# RUN: rm -rf %t && split-file %s %t
# RUN: yaml2obj %t/input.yaml -o %t/input.o
# RUN: ld.lld --gc-sections -T %t/layout.lds %t/input.o -o %t/out
# RUN: llvm-readobj --sections --symbols --relocations %t/out \
# RUN:   | FileCheck %s --check-prefix=LIVE \
# RUN:     --implicit-check-not='Name: dead (' --implicit-check-not=.text.dead
# RUN: llvm-objdump -s --section=.MMIX.reg_contents %t/out \
# RUN:   | FileCheck %s --check-prefix=REGS

# LIVE:      Name: .text.live
# LIVE:      Name: .text.kept
# LIVE:      Name: .MMIX.reg_contents
# LIVE:      Relocations [
# LIVE-NEXT: ]
# LIVE:      Name: __MMIX_call_stub_0
# LIVE:      Name: __MMIX_call_stub_1
# LIVE-NOT:  Name: __MMIX_call_stub_2
# REGS:      Contents of section .MMIX.reg_contents:
# REGS-NEXT: {{[0-9a-f]+}} 00000000 00000000 00000000 00000100

#--- layout.lds
ENTRY(live)
SECTIONS {
  .text.live 0x1000 : { *(.text.live) }
  .text.dead : { *(.text.dead) }
  .text.kept : { KEEP(*(.text.kept)) }
  .MMIX.reg_contents : {
    *(.MMIX.reg_contents.linker_allocated)
    *(.MMIX.reg_contents)
  }
}

#--- input.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .text.live
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content: F200000023AA0000
  - Name: .text.dead
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content: F200000023BB0000FD000000
  - Name: .text.kept
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content: F200000023CC0000
  - Name: .rela.text.live
    Type: SHT_RELA
    Link: .symtab
    Info: .text.live
    Relocations:
      - { Offset: 0, Type: R_MMIX_PUSHJ_STUBBABLE, Symbol: live_far }
      - { Offset: 6, Type: R_MMIX_BASE_PLUS_OFFSET, Symbol: low }
  - Name: .rela.text.dead
    Type: SHT_RELA
    Link: .symtab
    Info: .text.dead
    Relocations:
      - { Offset: 0, Type: R_MMIX_PUSHJ_STUBBABLE, Symbol: dead_far }
      - { Offset: 6, Type: R_MMIX_BASE_PLUS_OFFSET, Symbol: dead_base }
      - { Offset: 9, Type: R_MMIX_LOCAL, Symbol: dead_local }
  - Name: .rela.text.kept
    Type: SHT_RELA
    Link: .symtab
    Info: .text.kept
    Relocations:
      - { Offset: 0, Type: R_MMIX_PUSHJ_STUBBABLE, Symbol: kept_far }
      - { Offset: 6, Type: R_MMIX_BASE_PLUS_OFFSET, Symbol: high }
Symbols:
  - { Name: live, Section: .text.live, Binding: STB_GLOBAL }
  - { Name: dead, Section: .text.dead, Binding: STB_GLOBAL }
  - { Name: kept, Section: .text.kept, Binding: STB_GLOBAL }
  - { Name: live_far, Index: SHN_ABS, Value: 4294967296, Binding: STB_GLOBAL }
  - { Name: dead_far, Index: SHN_ABS, Value: 8589934592, Binding: STB_GLOBAL }
  - { Name: kept_far, Index: SHN_ABS, Value: 12884901888, Binding: STB_GLOBAL }
  - { Name: low, Index: SHN_ABS, Value: 0, Binding: STB_GLOBAL }
  - { Name: dead_base, Index: SHN_ABS, Value: 512, Binding: STB_GLOBAL }
  - { Name: dead_local, Index: SHN_ABS, Value: 252, Binding: STB_GLOBAL }
  - { Name: high, Index: SHN_ABS, Value: 256, Binding: STB_GLOBAL }
