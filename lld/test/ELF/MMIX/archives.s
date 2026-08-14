# REQUIRES: mmix

# RUN: rm -rf %t && split-file %s %t
# RUN: llvm-mc -triple=mmix -filetype=obj %t/main.s -o %t/main.o
# RUN: llvm-mc -triple=mmix -filetype=obj %t/relax.s -o %t/relax.o
# RUN: llvm-mc -triple=mmix -filetype=obj %t/call.s -o %t/call.o
# RUN: llvm-mc -triple=mmix -filetype=obj %t/unused.s -o %t/unused.o
# RUN: llvm-mc -triple=mmix -filetype=obj %t/weak.s -o %t/weak.o
# RUN: yaml2obj %t/bpo.yaml -o %t/bpo.o
# RUN: yaml2obj %t/reg.yaml -o %t/reg.o
# RUN: llvm-ar rcs %t/features-a.a %t/unused.o %t/weak.o %t/reg.o %t/call.o \
# RUN:   %t/bpo.o %t/relax.o
# RUN: llvm-ar rcs %t/features-b.a %t/relax.o %t/bpo.o %t/call.o %t/reg.o \
# RUN:   %t/weak.o %t/unused.o
# RUN: ld.lld -e _start %t/main.o %t/features-a.a -o %t/a
# RUN: ld.lld -e _start %t/main.o %t/features-b.a -o %t/b
# RUN: llvm-readobj --sections --symbols --relocations %t/a \
# RUN:   | FileCheck %s --check-prefix=FEATURES \
# RUN:     --implicit-check-not=unused_ --implicit-check-not=.text.unused \
# RUN:     --implicit-check-not=weak_archive_member
# RUN: llvm-readobj --sections --symbols --relocations %t/b \
# RUN:   | FileCheck %s --check-prefix=FEATURES \
# RUN:     --implicit-check-not=unused_ --implicit-check-not=.text.unused \
# RUN:     --implicit-check-not=weak_archive_member
# RUN: llvm-objdump -s --section=.text --section=.MMIX.reg_contents %t/a \
# RUN:   | FileCheck %s --check-prefix=CONTENTS
# RUN: llvm-objdump -s --section=.text --section=.MMIX.reg_contents %t/b \
# RUN:   | FileCheck %s --check-prefix=CONTENTS

# RUN: llvm-mc -triple=mmix -filetype=obj %t/group-main.s -o %t/group-main.o
# RUN: llvm-mc -triple=mmix -filetype=obj %t/group-a.s -o %t/group-a.o
# RUN: llvm-mc -triple=mmix -filetype=obj %t/group-b.s -o %t/group-b.o
# RUN: llvm-ar rcs %t/group-a.a %t/group-a.o
# RUN: llvm-ar rcs %t/group-b.a %t/group-b.o
# RUN: ld.lld -e group_start %t/group-main.o %t/group-b.a %t/group-a.a \
# RUN:   %t/group-b.a -o %t/repeated
# RUN: ld.lld -e group_start %t/group-main.o --start-group %t/group-b.a \
# RUN:   %t/group-a.a --end-group -o %t/grouped
# RUN: llvm-readobj --symbols %t/repeated | FileCheck %s --check-prefix=GROUP
# RUN: llvm-readobj --symbols %t/grouped | FileCheck %s --check-prefix=GROUP

# RUN: llvm-mc -triple=mmix -filetype=obj %t/whole.s -o %t/whole.o
# RUN: llvm-ar rcs %t/whole.a %t/whole.o
# RUN: ld.lld -e _start %t/main.o %t/features-a.a --whole-archive %t/whole.a \
# RUN:   --no-whole-archive -o %t/whole
# RUN: llvm-readobj --symbols %t/whole | FileCheck %s --check-prefix=WHOLE

# FEATURES:      Name: .MMIX.reg_contents
# FEATURES:      Relocations [
# FEATURES-NEXT: ]
# FEATURES:      Name: __MMIX_call_stub_0
# FEATURES:      Name: used_relax
# FEATURES:      Name: used_call
# FEATURES:      Name: used_bpo
# FEATURES:      Name: used_register
# CONTENTS:      Contents of section .MMIX.reg_contents:
# CONTENTS-NEXT: {{[0-9a-f]+}} 00000000 00000100 11223344 55667788
# GROUP:         Name: group_a
# GROUP:         Name: group_b
# WHOLE:         Name: whole_only

#--- main.s
.section .text.start,"ax",@progbits
.global _start
_start:
  GETA r1, %geta(used_relax)
  PUSHJ r2, used_call
  GETA r3, %geta(used_bpo)
  GETA r4, %geta(used_register)
.weak weak_only
  GETA r5, %geta(weak_only)

#--- relax.s
.section .text.relax,"ax",@progbits
.global used_relax
used_relax:
  SWYM 1, 0, 0

#--- call.s
.section .text.call,"ax",@progbits
.global used_call
used_call:
  PUSHJ r1, far_target
.set far_target, 0x1122334455667788

#--- unused.s
.section .text.unused,"ax",@progbits
.global unused_member
unused_member:
  PUSHJ r1, unused_far
.set unused_far, 0x8877665544332211
.section .MMIX.reg_contents,"",@progbits
.global unused_register
unused_register:
  .quad 0xaabbccddeeff0011

#--- weak.s
.section .text.weak,"ax",@progbits
.global weak_only
weak_only:
.global weak_archive_member
weak_archive_member:
  SWYM 0, 0, 0

#--- bpo.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .text.bpo
    Type: SHT_PROGBITS
    Flags: [ SHF_ALLOC, SHF_EXECINSTR ]
    AddressAlign: 4
    Content: 23AA0000
  - Name: .rela.text.bpo
    Type: SHT_RELA
    Link: .symtab
    Info: .text.bpo
    Relocations:
      - { Offset: 2, Type: R_MMIX_BASE_PLUS_OFFSET, Symbol: bpo_value }
Symbols:
  - { Name: used_bpo, Section: .text.bpo, Binding: STB_GLOBAL }
  - { Name: bpo_value, Index: SHN_ABS, Value: 256, Binding: STB_GLOBAL }

#--- reg.yaml
--- !ELF
FileHeader: { Class: ELFCLASS64, Data: ELFDATA2MSB, Type: ET_REL, Machine: EM_MMIX }
Sections:
  - Name: .MMIX.reg_contents
    Type: SHT_PROGBITS
    AddressAlign: 8
    Content: 1122334455667788
Symbols:
  - { Name: used_register, Section: .MMIX.reg_contents, Binding: STB_GLOBAL }

#--- group-main.s
.global group_start
group_start:
  GETA r1, %geta(group_a)

#--- group-a.s
.global group_a
group_a:
  GETA r1, %geta(group_b)

#--- group-b.s
.global group_b
group_b:
  SWYM 0, 0, 0

#--- whole.s
.section .text.whole,"ax",@progbits
.global whole_only
whole_only:
  SWYM 0, 0, 0
