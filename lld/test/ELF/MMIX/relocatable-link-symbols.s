# REQUIRES: mmix

# RUN: rm -rf %t && split-file %s %t
# RUN: llvm-mc -filetype=obj -triple=mmix %t/use.s -o %t/use.o
# RUN: llvm-mc -filetype=obj -triple=mmix %t/weak.s -o %t/weak.o
# RUN: llvm-mc -filetype=obj -triple=mmix %t/strong.s -o %t/strong.o
# RUN: llvm-mc -filetype=obj -triple=mmix %t/first.s -o %t/first.o
# RUN: llvm-mc -filetype=obj -triple=mmix %t/second.s -o %t/second.o
# RUN: ld.lld -r %t/use.o %t/weak.o %t/strong.o %t/first.o %t/second.o \
# RUN:   -o %t/partial.o
# RUN: llvm-readobj --sections --section-groups --symbols --relocations \
# RUN:   --expand-relocs %t/partial.o | FileCheck %s --check-prefix=PARTIAL \
# RUN:   --implicit-check-not=losing_comdat
# RUN: ld.lld -e selected %t/use.o %t/weak.o %t/strong.o %t/first.o \
# RUN:   %t/second.o -o %t/direct
# RUN: ld.lld -e selected %t/partial.o -o %t/staged
# RUN: llvm-objcopy --dump-section=.data=%t/direct.data %t/direct
# RUN: llvm-objcopy --dump-section=.data=%t/staged.data %t/staged
# RUN: cmp %t/direct.data %t/staged.data
# RUN: llvm-mc -filetype=obj -triple=mmix %t/duplicate.s -o %t/duplicate.o
# RUN: not ld.lld -r %t/strong.o %t/duplicate.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=DUPLICATE

# PARTIAL:      Name: .data
# PARTIAL:      Address: 0x0
# PARTIAL:      AddressAlignment: 16
# PARTIAL:      Relocation {
# PARTIAL:        Type: R_MMIX_64
# PARTIAL:        Symbol: selected
# PARTIAL:      Relocation {
# PARTIAL:        Type: R_MMIX_64
# PARTIAL:        Symbol: missing_weak
# PARTIAL:      Relocation {
# PARTIAL:        Type: R_MMIX_64
# PARTIAL:        Symbol: common_object
# PARTIAL:      Relocation {
# PARTIAL:        Type: R_MMIX_64
# PARTIAL:        Symbol: chosen
# PARTIAL:      Name: local_object
# PARTIAL:      Size: 8
# PARTIAL:      Binding: Local
# PARTIAL:      Type: Object
# PARTIAL:      Section: .data
# PARTIAL:      Name: hidden_object
# PARTIAL:      STV_HIDDEN
# PARTIAL:      Section: .data
# PARTIAL:      Name: selected
# PARTIAL:      Size: 4
# PARTIAL:      Binding: Global
# PARTIAL:      Type: Function
# PARTIAL:      Section: .text
# PARTIAL:      Name: protected_object
# PARTIAL:      STV_PROTECTED
# PARTIAL:      Section: .data
# PARTIAL:      Name: missing_weak
# PARTIAL:      Binding: Weak
# PARTIAL:      Section: Undefined
# PARTIAL:      Name: common_object
# PARTIAL:      Value: 0x10
# PARTIAL:      Size: 8
# PARTIAL:      Binding: Global
# PARTIAL:      Type: Object
# PARTIAL:      Section: Common
# PARTIAL:      Name: absolute_symbol
# PARTIAL:      Value: 0x1234
# PARTIAL:      Section: Absolute
# PARTIAL:      Name: chosen
# PARTIAL:      Size: 4
# PARTIAL:      Binding: Global
# PARTIAL:      Type: Function
# PARTIAL:      Section: .text.chosen
# PARTIAL:      Type: COMDAT
# PARTIAL:      Signature: chosen
# DUPLICATE: duplicate symbol: selected

#--- use.s
.section .data,"aw",@progbits
.p2align 4
.type local_object,@object
local_object:
  .quad 0x1122334455667788
.size local_object, .-local_object
.global hidden_object
.hidden hidden_object
hidden_object:
  .quad selected
.global protected_object
.protected protected_object
protected_object:
  .quad missing_weak
.weak missing_weak
.comm common_object,8,16
.global absolute_symbol
.set absolute_symbol,0x1234
  .quad common_object
  .quad chosen

#--- weak.s
.section .text,"ax",@progbits
.weak selected
.type selected,@function
selected:
  SWYM 0,0,1
.size selected,.-selected

#--- strong.s
.section .text,"ax",@progbits
.global selected
.type selected,@function
selected:
  SWYM 0,0,2
.size selected,.-selected

#--- first.s
.section .text.chosen,"axG",@progbits,chosen,comdat
.global chosen
.type chosen,@function
chosen:
  SWYM 0,0,3
.size chosen,.-chosen

#--- second.s
.section .text.chosen,"axG",@progbits,chosen,comdat
.global chosen
.type chosen,@function
chosen:
losing_comdat:
  SWYM 0,0,4
.size chosen,.-chosen

#--- duplicate.s
.global selected
selected:
  SWYM 0,0,5
