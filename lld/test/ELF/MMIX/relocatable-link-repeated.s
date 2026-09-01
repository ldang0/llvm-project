# REQUIRES: mmix

# RUN: rm -rf %t && split-file %s %t
# RUN: llvm-mc -filetype=obj -triple=mmix %t/prefix.s -o %t/prefix.o
# RUN: yaml2obj %t/use.yaml -o %t/use.o
# RUN: llvm-mc -filetype=obj -triple=mmix %t/definitions.s -o %t/definitions.o
# RUN: ld.lld -r %t/prefix.o %t/use.o -o %t/first.o
# RUN: ld.lld -r %t/first.o %t/definitions.o -o %t/repeated.o
# RUN: ld.lld -r %t/prefix.o %t/use.o %t/definitions.o -o %t/one-stage.o
# RUN: ld.lld -r %t/first.o %t/definitions.o -o %t/repeated-again.o
# RUN: cmp %t/repeated.o %t/repeated-again.o
# RUN: llvm-readobj --sections --symbols --relocations --expand-relocs \
# RUN:   %t/repeated.o | FileCheck %s --check-prefix=REPEATED
# RUN: ld.lld -e entry %t/prefix.o %t/use.o %t/definitions.o -o %t/direct
# RUN: ld.lld -e entry %t/one-stage.o -o %t/one-stage
# RUN: ld.lld -e entry %t/repeated.o -o %t/repeated
# RUN: llvm-readobj --relocations %t/repeated \
# RUN:   | FileCheck %s --check-prefix=FINAL --implicit-check-not=R_MMIX_
# RUN: llvm-objcopy --dump-section=.text=%t/direct.text \
# RUN:   --dump-section=.data=%t/direct.data \
# RUN:   --dump-section=.MMIX.reg_contents=%t/direct.regs %t/direct
# RUN: llvm-objcopy --dump-section=.text=%t/one.text \
# RUN:   --dump-section=.data=%t/one.data \
# RUN:   --dump-section=.MMIX.reg_contents=%t/one.regs %t/one-stage
# RUN: llvm-objcopy --dump-section=.text=%t/repeated.text \
# RUN:   --dump-section=.data=%t/repeated.data \
# RUN:   --dump-section=.MMIX.reg_contents=%t/repeated.regs %t/repeated
# RUN: cmp %t/direct.text %t/one.text
# RUN: cmp %t/direct.text %t/repeated.text
# RUN: cmp %t/direct.data %t/one.data
# RUN: cmp %t/direct.data %t/repeated.data
# RUN: cmp %t/direct.regs %t/one.regs
# RUN: cmp %t/direct.regs %t/repeated.regs

# REPEATED:      Name: .text
# REPEATED:      Size: 8
# REPEATED:      Name: .data
# REPEATED:      Size: 24
# REPEATED:      Name: .MMIX.reg_contents
# REPEATED:      Size: 8
# REPEATED:      Relocation {
# REPEATED:        Offset: 0x7
# REPEATED:        Type: R_MMIX_REG
# REPEATED:        Symbol: content_register
# REPEATED:      Relocation {
# REPEATED:        Offset: 0x8
# REPEATED:        Type: R_MMIX_64
# REPEATED:        Symbol: target
# REPEATED:      Name: content_register
# REPEATED-NEXT: Value: 0x0
# REPEATED:      Section: .MMIX.reg_contents
# FINAL:      Relocations [
# FINAL-NEXT: ]

#--- prefix.s
.section .text,"ax",@progbits
.global entry
entry:
  SWYM 0,0,1
.section .data,"aw",@progbits
  .quad 0x0102030405060708

#--- definitions.s
.section .data,"aw",@progbits
.global target
target:
  .quad 0x1122334455667788
.section .MMIX.reg_contents,"",@progbits
.p2align 3
.global content_register
content_register:
  .quad 0x99aabbccddeeff00

#--- use.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .text
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content: 22AA0000
  - Name: .data
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_WRITE ]
    AddressAlign: 8
    Content: 0000000000000000
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - { Offset: 3, Type: R_MMIX_REG, Symbol: content_register }
  - Name: .rela.data
    Type: SHT_RELA
    Link: .symtab
    Info: .data
    Relocations:
      - { Offset: 0, Type: R_MMIX_64, Symbol: target }
Symbols:
  - { Name: content_register, Index: SHN_UNDEF, Binding: STB_GLOBAL }
  - { Name: target, Index: SHN_UNDEF, Binding: STB_GLOBAL }
