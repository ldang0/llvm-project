# REQUIRES: mmix

# RUN: rm -rf %t && split-file %s %t
# RUN: llvm-mc -filetype=obj -triple=mmix %t/main.s -o %t/main.o
# RUN: llvm-mc -filetype=obj -triple=mmix %t/member.s -o %t/member.o
# RUN: llvm-mc -filetype=obj -triple=mmix %t/unused.s -o %t/unused.o
# RUN: llvm-ar rcs %t/libfeatures.a %t/unused.o %t/member.o
# RUN: ld.lld -r -T %t/partial.lds %t/main.o %t/libfeatures.a -o %t/partial.o
# RUN: llvm-readobj --file-headers --sections --symbols --relocations \
# RUN:   %t/partial.o | FileCheck %s --check-prefix=PARTIAL \
# RUN:   --implicit-check-not=unused_member --implicit-check-not=.text.drop
# RUN: llvm-mc -filetype=obj -triple=mmix %t/group-main.s -o %t/group-main.o
# RUN: llvm-mc -filetype=obj -triple=mmix %t/group-a.s -o %t/group-a.o
# RUN: llvm-mc -filetype=obj -triple=mmix %t/group-b.s -o %t/group-b.o
# RUN: llvm-ar rcs %t/liba.a %t/group-a.o
# RUN: llvm-ar rcs %t/libb.a %t/group-b.o
# RUN: ld.lld -r %t/group-main.o --start-group %t/libb.a %t/liba.a \
# RUN:   --end-group -o %t/grouped.o
# RUN: llvm-readobj --symbols %t/grouped.o | FileCheck %s --check-prefix=GROUP
# RUN: ld.lld -e archive_symbol -T %t/partial.lds %t/main.o %t/libfeatures.a \
# RUN:   -o %t/direct
# RUN: ld.lld -e archive_symbol %t/partial.o -o %t/staged
# RUN: llvm-objcopy --dump-section=.text.bundle=%t/direct.text %t/direct
# RUN: llvm-objcopy --dump-section=.text=%t/staged.text %t/staged
# RUN: cmp %t/direct.text %t/staged.text
# RUN: not ld.lld -r -T %t/malformed.lds %t/main.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=MALFORMED

# PARTIAL:      Type: Relocatable
# PARTIAL:      Entry: 0x0
# PARTIAL:      ProgramHeaderCount: 0
# PARTIAL:      Name: .text.bundle
# PARTIAL:      Type: SHT_PROGBITS
# PARTIAL:      SHF_ALLOC
# PARTIAL:      SHF_EXECINSTR
# PARTIAL:      Address: 0x0
# PARTIAL:      AddressAlignment: 16
# PARTIAL:      Name: archive_symbol
# PARTIAL:      Type: Function
# PARTIAL:      Section: .text.bundle
# PARTIAL:      Name: kept_symbol
# PARTIAL:      Section: .text.bundle
# GROUP:        Name: group_a
# GROUP:        Name: group_b
# MALFORMED:    malformed.lds:2: malformed number: 0xnot

#--- partial.lds
SECTIONS {
  .text.bundle : ALIGN(16) {
    *(.text.start)
    KEEP(*(.text.keep))
    *(.text.archive)
  }
  /DISCARD/ : { *(.text.drop) }
}

#--- malformed.lds
SECTIONS {
  .text 0xnot-a-number : { *(.text) }
}

#--- main.s
.section .text.start,"ax",@progbits
.global start
start:
  GETA r1,%geta(archive_symbol)
.section .text.keep,"ax",@progbits
.global kept_symbol
kept_symbol:
  SWYM 0,0,1
.section .text.drop,"ax",@progbits
.global dropped_symbol
dropped_symbol:
  SWYM 0,0,2

#--- member.s
.section .text.archive,"ax",@progbits
.global archive_symbol
.type archive_symbol,@function
archive_symbol:
  SWYM 0,0,3
.size archive_symbol,.-archive_symbol

#--- unused.s
.section .text.archive,"ax",@progbits
.global unused_member
unused_member:
  SWYM 0,0,4

#--- group-main.s
.global group_start
group_start:
  GETA r1,%geta(group_a)

#--- group-a.s
.global group_a
group_a:
  GETA r1,%geta(group_b)

#--- group-b.s
.global group_b
group_b:
  SWYM 0,0,5
