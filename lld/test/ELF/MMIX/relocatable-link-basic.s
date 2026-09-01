# REQUIRES: mmix

# RUN: rm -rf %t && split-file %s %t
# RUN: llvm-mc -filetype=obj -triple=mmix %t/a.s -o %t/a.o
# RUN: llvm-mc -filetype=obj -triple=mmix %t/b.s -o %t/b.o
# RUN: ld.lld -m elf64mmix -r %t/a.o %t/b.o -o %t/linked.o
# RUN: llvm-readobj --file-headers --sections --symbols --relocations \
# RUN:   %t/linked.o | FileCheck %s --implicit-check-not=.MMIX.reg_contents
# RUN: llvm-objdump -s --section=.text --section=.data %t/linked.o \
# RUN:   | FileCheck %s --check-prefix=CONTENTS

# CHECK:      Format: elf64-mmix
# CHECK:      Arch: mmix
# CHECK:      AddressSize: 64bit
# CHECK:      LoadName: <Not found>
# CHECK:      ElfHeader {
# CHECK:        Class: 64-bit
# CHECK:        DataEncoding: BigEndian
# CHECK:        Type: Relocatable
# CHECK:        Machine: EM_MMIX
# CHECK:        Entry: 0x0
# CHECK:        ProgramHeaderCount: 0
# CHECK:      }
# CHECK:      Name: .text
# CHECK:      Type: SHT_PROGBITS
# CHECK:      Flags [
# CHECK-DAG:    SHF_ALLOC
# CHECK-DAG:    SHF_EXECINSTR
# CHECK:      ]
# CHECK:      Address: 0x0
# CHECK:      Size: 8
# CHECK:      AddressAlignment: 4
# CHECK:      Name: .data
# CHECK:      Type: SHT_PROGBITS
# CHECK:      Flags [
# CHECK:        SHF_ALLOC
# CHECK:        SHF_WRITE
# CHECK:      ]
# CHECK:      Address: 0x0
# CHECK:      Size: 16
# CHECK:      AddressAlignment: 8
# CHECK:      Relocations [
# CHECK-NEXT: ]
# CHECK:      Name: local_data
# CHECK:      Value: 0x0
# CHECK:      Size: 8
# CHECK:      Binding: Local
# CHECK:      Type: Object
# CHECK:      Section: .data
# CHECK:      Name: first
# CHECK:      Value: 0x0
# CHECK:      Size: 4
# CHECK:      Binding: Global
# CHECK:      Type: Function
# CHECK:      Section: .text
# CHECK:      Name: first_data
# CHECK:      Value: 0x0
# CHECK:      Size: 8
# CHECK:      Binding: Global
# CHECK:      Type: Object
# CHECK:      Section: .data
# CHECK:      Name: unresolved_data
# CHECK:      Value: 0x0
# CHECK:      Size: 0
# CHECK:      Binding: Weak
# CHECK:      Type: Object
# CHECK:      Section: Undefined
# CHECK:      Name: second
# CHECK:      Value: 0x4
# CHECK:      Size: 4
# CHECK:      Binding: Global
# CHECK:      Type: Function
# CHECK:      Section: .text
# CHECK:      Name: second_data
# CHECK:      Value: 0x8
# CHECK:      Size: 8
# CHECK:      Binding: Global
# CHECK:      Type: Object
# CHECK:      Section: .data

# CONTENTS:      Contents of section .text:
# CONTENTS-NEXT: 0000 fd000001 fd000002
# CONTENTS:      Contents of section .data:
# CONTENTS-NEXT: 0000 00000000 00000001 00000000 00000002

#--- a.s
.text
.globl first
.type first,@function
first:
  SWYM 0, 0, 1
.size first, .-first

.data
.p2align 3
.globl first_data
.type first_data,@object
.local local_data
.type local_data,@object
first_data:
local_data:
  .quad 1
.size first_data, .-first_data
.size local_data, .-local_data

.weak unresolved_data
.type unresolved_data,@object

#--- b.s
.text
.globl second
.type second,@function
second:
  SWYM 0, 0, 2
.size second, .-second

.data
.p2align 3
.globl second_data
.type second_data,@object
second_data:
  .quad 2
.size second_data, .-second_data
