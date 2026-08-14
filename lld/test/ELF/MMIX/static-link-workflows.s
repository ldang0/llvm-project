# REQUIRES: mmix

# RUN: rm -rf %t && split-file %s %t
# RUN: llvm-mc -triple=mmix -filetype=obj %t/main.s -o %t/main.o
# RUN: llvm-mc -triple=mmix -filetype=obj %t/chosen.s -o %t/chosen.o
# RUN: llvm-mc -triple=mmix -filetype=obj %t/losing.s -o %t/losing.o
# RUN: llvm-mc -triple=mmix -filetype=obj %t/keep.s -o %t/keep.o
# RUN: yaml2obj %t/feature.yaml -o %t/feature.o
# RUN: yaml2obj %t/register.yaml -o %t/register.o
# RUN: yaml2obj %t/unused.yaml -o %t/unused.o
# RUN: yaml2obj %t/dead.yaml -o %t/dead.o
# RUN: llvm-ar rcs %t/features.a %t/feature.o %t/register.o %t/unused.o
# RUN: ld.lld --gc-sections -T %t/layout.lds -Map=%t/one.map %t/main.o \
# RUN:   %t/chosen.o %t/losing.o %t/keep.o %t/dead.o %t/features.a -o %t/one
# RUN: ld.lld --gc-sections -T %t/layout.lds -Map=%t/two.map %t/main.o \
# RUN:   %t/chosen.o %t/losing.o %t/keep.o %t/dead.o %t/features.a -o %t/two
# RUN: cmp %t/one %t/two
# RUN: cmp %t/one.map %t/two.map
# RUN: llvm-readobj --sections --symbols --relocations %t/one \
# RUN:   | FileCheck %s --check-prefix=OBJECT \
# RUN:     --implicit-check-not=dead_entry \
# RUN:     --implicit-check-not=losing_marker \
# RUN:     --implicit-check-not=unused_member
# RUN: llvm-objdump -s --section=.MMIX.reg_contents %t/one \
# RUN:   | FileCheck %s --check-prefix=REGISTERS
# RUN: llvm-objdump --no-print-imm-hex -d %t/one \
# RUN:   | FileCheck %s --check-prefix=DISASSEMBLY

# OBJECT:      Name: .text
# OBJECT:      Name: .MMIX.reg_contents
# OBJECT:      Relocations [
# OBJECT-NEXT: ]
# OBJECT:      Name: __MMIX_call_stub_0
# OBJECT-NOT:  Name: __MMIX_call_stub_1
# OBJECT:      Name: _start
# OBJECT:      Name: archive_entry
# OBJECT:      Name: chosen
# OBJECT:      Name: weak_missing
# OBJECT-NEXT: Value: 0x0
# OBJECT-NEXT: Size: 0
# OBJECT-NEXT: Binding: Weak
# OBJECT-NEXT: Type: None
# OBJECT-NEXT: Other: 0
# OBJECT-NEXT: Section: Undefined
# OBJECT:      Name: kept_entry
# OBJECT:      Name: fixed_register
# REGISTERS:      Contents of section .MMIX.reg_contents:
# REGISTERS-NEXT: {{[0-9a-f]+}} 00000000 00000100 11223344 55667788
# DISASSEMBLY-LABEL: <_start>:
# DISASSEMBLY:       PUSHJ r1,
# DISASSEMBLY:       GETA r2,
# DISASSEMBLY-LABEL: <archive_entry>:
# DISASSEMBLY:       GETA r0,
# DISASSEMBLY:       PUSHJ r0,
# DISASSEMBLY-LABEL: <__MMIX_call_stub_0>:
# DISASSEMBLY-LABEL: <chosen>:
# DISASSEMBLY-LABEL: <kept_entry>:

#--- main.s
.section .text.start,"ax",@progbits
.global _start
_start:
  PUSHJ r1, archive_entry
  GETA r2, %geta(chosen)
.weak weak_missing
  GETA r3, %geta(weak_missing)

#--- chosen.s
.section .text.chosen,"axG",@progbits,chosen,comdat
.global chosen
.type chosen,@function
chosen:
  SWYM 1, 0, 0

#--- losing.s
.section .text.chosen,"axG",@progbits,chosen,comdat
.global chosen
.type chosen,@function
chosen:
losing_marker:
  SWYM 2, 0, 0

#--- keep.s
.section .text.keep,"ax",@progbits
.global kept_entry
kept_entry:
  SWYM 3, 0, 0

#--- layout.lds
ENTRY(_start)
SECTIONS {
  .text 0x1000 : {
    *(.text.start)
    *(.text.feature)
    *(.text.chosen)
    KEEP(*(.text.keep))
  }
  .MMIX.reg_contents 0x200000 : {
    *(.MMIX.reg_contents.linker_allocated)
    *(.MMIX.reg_contents)
  }
}

#--- feature.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .text.feature
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content: F4000000FD000000FD000000FD000000F200000023AA000000000000
  - Name: .rela.text.feature
    Type: SHT_RELA
    Link: .symtab
    Info: .text.feature
    Relocations:
      - { Offset: 0, Type: R_MMIX_GETA, Symbol: chosen }
      - { Offset: 16, Type: R_MMIX_PUSHJ_STUBBABLE, Symbol: far_target }
      - { Offset: 22, Type: R_MMIX_BASE_PLUS_OFFSET, Symbol: base_value }
      - { Offset: 24, Type: R_MMIX_REG, Symbol: fixed_register }
      - { Offset: 25, Type: R_MMIX_REG_OR_BYTE, Symbol: byte_value }
      - { Offset: 26, Type: R_MMIX_LOCAL, Symbol: local_value }
Symbols:
  - { Name: archive_entry, Section: .text.feature, Binding: STB_GLOBAL, Type: STT_FUNC }
  - { Name: chosen, Binding: STB_GLOBAL }
  - { Name: fixed_register, Binding: STB_GLOBAL }
  - { Name: far_target, Index: SHN_ABS, Value: 0x1122334455667788, Binding: STB_GLOBAL }
  - { Name: base_value, Index: SHN_ABS, Value: 256, Binding: STB_GLOBAL }
  - { Name: byte_value, Index: SHN_ABS, Value: 7, Binding: STB_GLOBAL }
  - { Name: local_value, Index: SHN_ABS, Value: 200, Binding: STB_GLOBAL }

#--- register.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .MMIX.reg_contents
    Type: SHT_PROGBITS
    AddressAlign: 8
    Content: 1122334455667788
Symbols:
  - { Name: fixed_register, Section: .MMIX.reg_contents, Binding: STB_GLOBAL }

#--- unused.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .text.unused
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content: FD000000
  - Name: .MMIX.reg_contents
    Type: SHT_PROGBITS
    AddressAlign: 8
    Content: AABBCCDDEEFF0011
Symbols:
  - { Name: unused_member, Section: .text.unused, Binding: STB_GLOBAL }
  - { Name: unused_register, Section: .MMIX.reg_contents, Binding: STB_GLOBAL }

#--- dead.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .text.dead
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content: F200000023BB0000
  - Name: .rela.text.dead
    Type: SHT_RELA
    Link: .symtab
    Info: .text.dead
    Relocations:
      - { Offset: 0, Type: R_MMIX_PUSHJ_STUBBABLE, Symbol: dead_far }
      - { Offset: 6, Type: R_MMIX_BASE_PLUS_OFFSET, Symbol: dead_base }
      - { Offset: 7, Type: R_MMIX_LOCAL, Symbol: dead_local }
Symbols:
  - { Name: dead_entry, Section: .text.dead, Binding: STB_GLOBAL }
  - { Name: dead_far, Index: SHN_ABS, Value: 0x8877665544332210, Binding: STB_GLOBAL }
  - { Name: dead_base, Index: SHN_ABS, Value: 512, Binding: STB_GLOBAL }
  - { Name: dead_local, Index: SHN_ABS, Value: 201, Binding: STB_GLOBAL }
