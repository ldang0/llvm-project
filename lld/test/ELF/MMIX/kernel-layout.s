# REQUIRES: mmix

## Exercise the reusable static layout required by a growing kernel image.
# RUN: rm -rf %t && split-file %s %t
# RUN: yaml2obj %t/entry.yaml -o %t/entry.o
# RUN: llvm-mc -triple=mmix -filetype=obj %t/targets.s -o %t/targets.o
# RUN: llvm-mc -triple=mmix -filetype=obj %t/data.s -o %t/data.o
# RUN: ld.lld -static -T %t/kernel.lds %t/entry.o %t/targets.o %t/data.o \
# RUN:   -o %t/kernel
# RUN: llvm-readobj --file-headers --sections --program-headers --symbols \
# RUN:   --relocations %t/kernel | FileCheck %s --check-prefix=STRUCTURE \
# RUN:   --implicit-check-not=R_MMIX_
# RUN: llvm-objdump --no-print-imm-hex -d --section=.text %t/kernel \
# RUN:   | FileCheck %s --check-prefix=DIS
# RUN: llvm-objdump -s --section=.data %t/kernel \
# RUN:   | FileCheck %s --check-prefix=DATA

# STRUCTURE:      Format: elf64-mmix
# STRUCTURE:      Type: Executable
# STRUCTURE:      Entry: 0x100000
# STRUCTURE:      Name: .text
# STRUCTURE-NEXT: Type: SHT_PROGBITS
# STRUCTURE:      Flags [
# STRUCTURE:      SHF_ALLOC
# STRUCTURE:      SHF_EXECINSTR
# STRUCTURE:      Address: 0x100000
# STRUCTURE:      Name: .rodata
# STRUCTURE:      Address: 0x202000
# STRUCTURE:      Name: .data
# STRUCTURE:      Address: 0x204000
# STRUCTURE:      Name: .bss
# STRUCTURE:      Address: 0x206000
# STRUCTURE:      Type: PT_LOAD
# STRUCTURE:      VirtualAddress: 0x100000
# STRUCTURE:      Flags [
# STRUCTURE:      PF_R
# STRUCTURE:      PF_X
# STRUCTURE:      Type: PT_LOAD
# STRUCTURE:      VirtualAddress: 0x202000
# STRUCTURE:      Flags [
# STRUCTURE:      PF_R
# STRUCTURE-NOT:  PF_W
# STRUCTURE-NOT:  PF_X
# STRUCTURE:      Type: PT_LOAD
# STRUCTURE:      VirtualAddress: 0x204000
# STRUCTURE:      Flags [
# STRUCTURE:      PF_R
# STRUCTURE:      PF_W
# STRUCTURE:      Relocations [
# STRUCTURE-NEXT: ]
# STRUCTURE:      Name: _entry
# STRUCTURE-NEXT: Value: 0x100000
# STRUCTURE:      Name: kernel_end
# STRUCTURE-NEXT: Value: 0x208000
# STRUCTURE:      Name: text_start
# STRUCTURE-NEXT: Value: 0x100000
# STRUCTURE:      Name: text_end
# STRUCTURE-NEXT: Value: 0x200008
# STRUCTURE:      Name: rodata_start
# STRUCTURE-NEXT: Value: 0x202000
# STRUCTURE:      Name: rodata_end
# STRUCTURE-NEXT: Value: 0x20200C
# STRUCTURE:      Name: data_start
# STRUCTURE-NEXT: Value: 0x204000
# STRUCTURE:      Name: data_end
# STRUCTURE-NEXT: Value: 0x204010
# STRUCTURE:      Name: bss_start
# STRUCTURE-NEXT: Value: 0x206000
# STRUCTURE:      Name: bss_end
# STRUCTURE-NEXT: Value: 0x208000

# DIS-LABEL: <text_start>:
# DIS:       BZ r1, 11
# DIS-NEXT:  PUSHJ r31, 11
# DIS-NEXT:  PBNZ r1, 6
# DIS-NEXT:  SETL r255, 0
# DIS-NEXT:  INCML r255, 32
# DIS-NEXT:  INCMH r255, 0
# DIS-NEXT:  INCH r255, 0
# DIS-NEXT:  GO r255, r255, 0
# DIS-NEXT:  PUSHJ r31, 2
# DIS-LABEL: <__MMIX_call_stub_0>:
# DIS-NEXT:  JMP 262135
# DIS-LABEL: <far_branch_target>:
# DIS-NEXT:  SWYM 0, 0, 0
# DIS-LABEL: <far_call_target>:
# DIS-NEXT:  POP 0, 0

# DATA:      Contents of section .data:
# DATA-NEXT: 204000 11223344 55667788 00000000 00202000

#--- entry.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .text.entry
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content: 42010000F21F000042010000FD000000FD000000FD000000FD000000FD000000F21F0000F0000000
  - Name: .rela.text.entry
    Type: SHT_RELA
    Link: .symtab
    Info: .text.entry
    Relocations:
      - { Offset: 0, Type: R_MMIX_ADDR19, Symbol: near_branch_target }
      - { Offset: 4, Type: R_MMIX_PUSHJ_STUBBABLE, Symbol: near_call_target }
      - { Offset: 8, Type: R_MMIX_CBRANCH, Symbol: far_branch_target }
      - { Offset: 32, Type: R_MMIX_PUSHJ_STUBBABLE, Symbol: far_call_target }
Symbols:
  - { Name: _entry, Section: .text.entry, Binding: STB_GLOBAL, Type: STT_FUNC, Size: 40 }
  - { Name: near_branch_target, Binding: STB_GLOBAL }
  - { Name: near_call_target, Binding: STB_GLOBAL }
  - { Name: far_branch_target, Binding: STB_GLOBAL }
  - { Name: far_call_target, Binding: STB_GLOBAL }

#--- targets.s
.section .text.near,"ax",@progbits
.global near_branch_target
.type near_branch_target,@function
near_branch_target:
  SWYM 0, 0, 0
.size near_branch_target, .-near_branch_target

.global near_call_target
.type near_call_target,@function
near_call_target:
  POP 0, 0
.size near_call_target, .-near_call_target

.section .text.far,"ax",@progbits
.global far_branch_target
.type far_branch_target,@function
far_branch_target:
  SWYM 0, 0, 0
.size far_branch_target, .-far_branch_target

.global far_call_target
.type far_call_target,@function
far_call_target:
  POP 0, 0
.size far_call_target, .-far_call_target

#--- data.s
.section .rodata.kernel,"a",@progbits
.global kernel_banner
.type kernel_banner,@object
kernel_banner:
  .asciz "MMIX kernel"
.size kernel_banner, .-kernel_banner

.section .data.kernel,"aw",@progbits
.global kernel_state
.type kernel_state,@object
kernel_state:
  .quad 0x1122334455667788
  .quad kernel_banner
.size kernel_state, .-kernel_state

.section .bss.kernel,"aw",@nobits
.balign 8192
.global kernel_workspace
.type kernel_workspace,@object
kernel_workspace:
  .space 8192
.size kernel_workspace, .-kernel_workspace

#--- kernel.lds
ENTRY(_entry)

PHDRS {
  text PT_LOAD FLAGS(5);
  rodata PT_LOAD FLAGS(4);
  data PT_LOAD FLAGS(6);
}

SECTIONS {
  . = 0x00100000;
  text_start = .;
  .text : ALIGN(0x2000) {
    KEEP(*(.text.entry))
    *(.text.near)
    . = 0x200000;
    *(.text.far)
  } :text
  text_end = .;

  .rodata : ALIGN(0x2000) { *(.rodata .rodata.*) } :rodata
  rodata_start = ADDR(.rodata);
  rodata_end = .;

  .data : ALIGN(0x2000) { *(.data .data.*) } :data
  data_start = ADDR(.data);
  data_end = .;

  .bss (NOLOAD) : ALIGN(0x2000) { *(.bss .bss.*) *(COMMON) } :data
  bss_start = ADDR(.bss);
  bss_end = .;
  . = ALIGN(0x2000);
  kernel_end = .;

  ASSERT(_entry == 0x00100000, "kernel entry moved")
  ASSERT(kernel_end <= 0x05ffe000, "kernel exceeds MMIX Low RAM")
  /DISCARD/ : { *(.comment) *(.note.GNU-stack) }
}
