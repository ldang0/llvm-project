# REQUIRES: mmix

# RUN: rm -rf %t && split-file %s %t
# RUN: llvm-mc -filetype=obj -triple=mmix %t/prefix.s -o %t/prefix.o
# RUN: yaml2obj %t/relocations.yaml -o %t/relocations.o
# RUN: llvm-mc -filetype=obj -triple=mmix %t/definitions.s -o %t/definitions.o
# RUN: ld.lld -m elf64mmix -r %t/prefix.o %t/relocations.o -o %t/partial.o
# RUN: llvm-readobj --file-headers --relocations --expand-relocs %t/partial.o \
# RUN:   | FileCheck %s --check-prefix=PARTIAL
# RUN: llvm-objdump -s --section=.text --section=.data %t/partial.o \
# RUN:   | FileCheck %s --check-prefix=CONTENTS
# RUN: ld.lld -T %t/layout.lds %t/prefix.o %t/relocations.o \
# RUN:   %t/definitions.o -o %t/direct
# RUN: ld.lld -T %t/layout.lds %t/partial.o %t/definitions.o -o %t/staged
# RUN: llvm-readobj --file-headers --symbols --relocations %t/direct \
# RUN:   | FileCheck %s --check-prefix=FINAL --implicit-check-not=R_MMIX_
# RUN: llvm-readobj --file-headers --symbols --relocations %t/staged \
# RUN:   | FileCheck %s --check-prefix=FINAL --implicit-check-not=R_MMIX_
# RUN: llvm-objcopy --dump-section=.text=%t/direct.text \
# RUN:   --dump-section=.data=%t/direct.data %t/direct
# RUN: llvm-objcopy --dump-section=.text=%t/staged.text \
# RUN:   --dump-section=.data=%t/staged.data %t/staged
# RUN: cmp %t/direct.text %t/staged.text
# RUN: cmp %t/direct.data %t/staged.data

# PARTIAL:      Format: elf64-mmix
# PARTIAL:      Type: Relocatable
# PARTIAL:      Entry: 0x0
# PARTIAL:      ProgramHeaderCount: 0
# PARTIAL:      Section {{.*}} .rela.text {
# PARTIAL:        Relocation {
# PARTIAL:          Offset: 0x8
# PARTIAL:          Type: R_MMIX_GETA
# PARTIAL:          Symbol: control_target
# PARTIAL:          Addend: 0x7
# PARTIAL:        Relocation {
# PARTIAL:          Offset: 0x18
# PARTIAL:          Type: R_MMIX_CBRANCH
# PARTIAL:          Symbol: control_target
# PARTIAL:          Addend: 0xFFFFFFFFFFFFFFF8
# PARTIAL:        Relocation {
# PARTIAL:          Offset: 0x30
# PARTIAL:          Type: R_MMIX_PUSHJ
# PARTIAL:          Symbol: control_target
# PARTIAL:          Addend: 0xC
# PARTIAL:        Relocation {
# PARTIAL:          Offset: 0x44
# PARTIAL:          Type: R_MMIX_JMP
# PARTIAL:          Symbol: control_target
# PARTIAL:          Addend: 0xFFFFFFFFFFFFFFF0
# PARTIAL:        Relocation {
# PARTIAL:          Offset: 0x5C
# PARTIAL:          Type: R_MMIX_ADDR19
# PARTIAL:          Symbol: control_target
# PARTIAL:          Addend: 0x14
# PARTIAL:        Relocation {
# PARTIAL:          Offset: 0x60
# PARTIAL:          Type: R_MMIX_ADDR27
# PARTIAL:          Symbol: control_target
# PARTIAL:          Addend: 0xFFFFFFFFFFFFFFE8
# PARTIAL:        Relocation {
# PARTIAL:          Offset: 0x64
# PARTIAL:          Type: R_MMIX_PUSHJ_STUBBABLE
# PARTIAL:          Symbol: control_target
# PARTIAL:          Addend: 0x1C
# PARTIAL:      Section {{.*}} .rela.data {
# PARTIAL:        Relocation {
# PARTIAL:          Offset: 0x8
# PARTIAL:          Type: R_MMIX_NONE
# PARTIAL:          Symbol: abs_target
# PARTIAL:          Addend: 0x1
# PARTIAL:        Relocation {
# PARTIAL:          Offset: 0xC
# PARTIAL:          Type: R_MMIX_8
# PARTIAL:          Symbol: abs_target
# PARTIAL:          Addend: 0x2
# PARTIAL:        Relocation {
# PARTIAL:          Offset: 0xD
# PARTIAL:          Type: R_MMIX_16
# PARTIAL:          Symbol: abs_target
# PARTIAL:          Addend: 0xFFFFFFFFFFFFFFFD
# PARTIAL:        Relocation {
# PARTIAL:          Offset: 0xF
# PARTIAL:          Type: R_MMIX_24
# PARTIAL:          Symbol: abs_target
# PARTIAL:          Addend: 0x4
# PARTIAL:        Relocation {
# PARTIAL:          Offset: 0x13
# PARTIAL:          Type: R_MMIX_32
# PARTIAL:          Symbol: abs_target
# PARTIAL:          Addend: 0xFFFFFFFFFFFFFFFB
# PARTIAL:        Relocation {
# PARTIAL:          Offset: 0x17
# PARTIAL:          Type: R_MMIX_64
# PARTIAL:          Symbol: abs_target
# PARTIAL:          Addend: 0x6
# PARTIAL:        Relocation {
# PARTIAL:          Offset: 0x1F
# PARTIAL:          Type: R_MMIX_PC_8
# PARTIAL:          Symbol: data_target
# PARTIAL:          Addend: 0xFFFFFFFFFFFFFFF9
# PARTIAL:        Relocation {
# PARTIAL:          Offset: 0x20
# PARTIAL:          Type: R_MMIX_PC_16
# PARTIAL:          Symbol: data_target
# PARTIAL:          Addend: 0x8
# PARTIAL:        Relocation {
# PARTIAL:          Offset: 0x22
# PARTIAL:          Type: R_MMIX_PC_24
# PARTIAL:          Symbol: data_target
# PARTIAL:          Addend: 0xFFFFFFFFFFFFFFF7
# PARTIAL:        Relocation {
# PARTIAL:          Offset: 0x26
# PARTIAL:          Type: R_MMIX_PC_32
# PARTIAL:          Symbol: data_target
# PARTIAL:          Addend: 0xA
# PARTIAL:        Relocation {
# PARTIAL:          Offset: 0x2A
# PARTIAL:          Type: R_MMIX_PC_64
# PARTIAL:          Symbol: data_target
# PARTIAL:          Addend: 0xFFFFFFFFFFFFFFF5

## The prefix bytes precede unchanged relocation fields. No range check,
## relaxation, expansion, or stub creation is permitted in the partial link.
# CONTENTS:      Contents of section .text:
# CONTENTS-NEXT: 0000 fd000001 fd000002 f4010000 fd000000
# CONTENTS-NEXT: 0010 fd000000 fd000000 40010000 fd000000
# CONTENTS-NEXT: 0020 fd000000 fd000000 fd000000 fd000000
# CONTENTS-NEXT: 0030 f2010000 fd000000 fd000000 fd000000
# CONTENTS-NEXT: 0040 fd000000 f0000000 fd000000 fd000000
# CONTENTS-NEXT: 0050 fd000000 fd000000 fd000000 42010000
# CONTENTS-NEXT: 0060 f0000000 f21f0000
# CONTENTS:      Contents of section .data:
# CONTENTS-NEXT: 0000 01020304 05060708 deadbeef 00000000
# CONTENTS-NEXT: 0010 00000000 00000000 00000000 00000000
# CONTENTS-NEXT: 0020 00000000 00000000 00000000
# CONTENTS-NEXT: 0030 0000

# FINAL:      Format: elf64-mmix
# FINAL:      Type: Executable
# FINAL:      Entry: 0x10068
# FINAL:      Relocations [
# FINAL-NEXT: ]
# FINAL:      Name: control_target
# FINAL:      Value: 0x10068
# FINAL:      Name: data_target
# FINAL:      Value: 0x20032

#--- prefix.s
.text
  SWYM 0, 0, 1
  SWYM 0, 0, 2
.data
  .quad 0x0102030405060708

#--- definitions.s
.section .text.targets,"ax",@progbits
.globl control_target
.type control_target,@function
control_target:
  SWYM 0, 0, 0
.size control_target, .-control_target

.section .data.targets,"aw",@progbits
.globl data_target
.type data_target,@object
data_target:
  .quad 0xcafe
.size data_target, .-data_target

.set abs_target, 0x12
.globl abs_target

#--- layout.lds
ENTRY(control_target)
SECTIONS {
  .text 0x10000 : { *(.text) *(.text.targets) }
  .data 0x20000 : { *(.data) *(.data.targets) }
}

#--- relocations.yaml
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
    Content:      F4010000FD000000FD000000FD00000040010000FD000000FD000000FD000000FD000000FD000000F2010000FD000000FD000000FD000000FD000000F0000000FD000000FD000000FD000000FD000000FD00000042010000F0000000F21F0000
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - { Offset: 0,  Type: R_MMIX_GETA, Symbol: control_target, Addend: 7 }
      - { Offset: 16, Type: R_MMIX_CBRANCH, Symbol: control_target, Addend: -8 }
      - { Offset: 40, Type: R_MMIX_PUSHJ, Symbol: control_target, Addend: 12 }
      - { Offset: 60, Type: R_MMIX_JMP, Symbol: control_target, Addend: -16 }
      - { Offset: 84, Type: R_MMIX_ADDR19, Symbol: control_target, Addend: 20 }
      - { Offset: 88, Type: R_MMIX_ADDR27, Symbol: control_target, Addend: -24 }
      - { Offset: 92, Type: R_MMIX_PUSHJ_STUBBABLE, Symbol: control_target, Addend: 28 }
  - Name:         .data
    Type:         SHT_PROGBITS
    Flags:        [ SHF_ALLOC, SHF_WRITE ]
    AddressAlign: 1
    Content:      DEADBEEF0000000000000000000000000000000000000000000000000000000000000000000000000000
  - Name: .rela.data
    Type: SHT_RELA
    Link: .symtab
    Info: .data
    Relocations:
      - { Offset: 0,  Type: R_MMIX_NONE, Symbol: abs_target, Addend: 1 }
      - { Offset: 4,  Type: R_MMIX_8, Symbol: abs_target, Addend: 2 }
      - { Offset: 5,  Type: R_MMIX_16, Symbol: abs_target, Addend: -3 }
      - { Offset: 7,  Type: R_MMIX_24, Symbol: abs_target, Addend: 4 }
      - { Offset: 11, Type: R_MMIX_32, Symbol: abs_target, Addend: -5 }
      - { Offset: 15, Type: R_MMIX_64, Symbol: abs_target, Addend: 6 }
      - { Offset: 23, Type: R_MMIX_PC_8, Symbol: data_target, Addend: -7 }
      - { Offset: 24, Type: R_MMIX_PC_16, Symbol: data_target, Addend: 8 }
      - { Offset: 26, Type: R_MMIX_PC_24, Symbol: data_target, Addend: -9 }
      - { Offset: 30, Type: R_MMIX_PC_32, Symbol: data_target, Addend: 10 }
      - { Offset: 34, Type: R_MMIX_PC_64, Symbol: data_target, Addend: -11 }
Symbols:
  - { Name: control_target, Binding: STB_GLOBAL }
  - { Name: data_target, Binding: STB_GLOBAL }
  - { Name: abs_target, Binding: STB_GLOBAL }
