# REQUIRES: mmix

# RUN: rm -rf %t && split-file %s %t
# RUN: llvm-mc -triple=mmix -filetype=obj %t/main.s -o %t/main.o
# RUN: ld.lld -T %t/layout.lds %t/main.o -o %t/exe
# RUN: llvm-readobj --file-headers %t/exe | FileCheck %s --check-prefix=HEADER
# RUN: llvm-readobj --sections %t/exe | FileCheck %s --check-prefix=SECTIONS
# RUN: llvm-readobj --symbols %t/exe | FileCheck %s --check-prefix=SYMBOLS
# RUN: llvm-readobj --elf-output-style=GNU --sections --program-headers \
# RUN:   %t/exe | FileCheck %s --check-prefix=SEGMENTS
# RUN: llvm-objdump -s --section=.data %t/exe \
# RUN:   | FileCheck %s --check-prefix=DATA
# RUN: llvm-mc -triple=mmix -filetype=obj %t/duplicate.s -o %t/duplicate.o
# RUN: not ld.lld -e _start %t/main.o %t/duplicate.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=DUPLICATE
# RUN: llvm-mc -triple=mmix -filetype=obj %t/unresolved.s -o %t/unresolved.o
# RUN: not ld.lld -e unresolved_entry %t/unresolved.o -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=UNRESOLVED

# HEADER:      Type: Executable
# HEADER:      Entry: 0x1000

# SECTIONS-LABEL: Name: .text
# SECTIONS:       Type: SHT_PROGBITS
# SECTIONS:       SHF_ALLOC
# SECTIONS:       SHF_EXECINSTR
# SECTIONS:       Address: 0x1000
# SECTIONS:       AddressAlignment: 16
# SECTIONS-LABEL: Name: .rodata
# SECTIONS:       Type: SHT_PROGBITS
# SECTIONS:       SHF_ALLOC
# SECTIONS:       Address: 0x2000
# SECTIONS:       AddressAlignment: 32
# SECTIONS-LABEL: Name: .data
# SECTIONS:       Type: SHT_PROGBITS
# SECTIONS:       SHF_ALLOC
# SECTIONS:       SHF_WRITE
# SECTIONS:       Address: 0x3000
# SECTIONS:       AddressAlignment: 32
# SECTIONS-LABEL: Name: .orphan
# SECTIONS:       Type: SHT_PROGBITS
# SECTIONS:       SHF_ALLOC
# SECTIONS:       SHF_WRITE
# SECTIONS:       AddressAlignment: 16
# SECTIONS-LABEL: Name: .bss
# SECTIONS:       Type: SHT_NOBITS
# SECTIONS:       SHF_ALLOC
# SECTIONS:       SHF_WRITE
# SECTIONS:       Address: 0x4000
# SECTIONS:       AddressAlignment: 64
# SECTIONS-LABEL: Name: .note.mmix
# SECTIONS:       Type: SHT_NOTE
# SECTIONS-NOT:   SHF_ALLOC

# SYMBOLS:      Name: local_function
# SYMBOLS:      Binding: Local
# SYMBOLS:      Type: Function
# SYMBOLS:      Section: .text
# SYMBOLS:      Name: local_rodata
# SYMBOLS:      Value: 0x2000
# SYMBOLS:      Binding: Local
# SYMBOLS:      Section: .rodata
# SYMBOLS:      Name: hidden_function
# SYMBOLS:      Value: 0x100C
# SYMBOLS:      Binding: Local
# SYMBOLS:      STV_HIDDEN
# SYMBOLS:      Name: _start
# SYMBOLS:      Value: 0x1000
# SYMBOLS:      Binding: Global
# SYMBOLS:      Type: Function
# SYMBOLS:      Name: global_function
# SYMBOLS:      Value: 0x1008
# SYMBOLS:      Type: Function
# SYMBOLS:      Name: weak_function
# SYMBOLS:      Value: 0x1010
# SYMBOLS:      Binding: Weak
# SYMBOLS:      Name: global_object
# SYMBOLS:      Value: 0x2008
# SYMBOLS:      Type: Object
# SYMBOLS:      Name: object_alias
# SYMBOLS:      Value: 0x2008
# SYMBOLS:      Type: Object
# SYMBOLS:      Name: weak_undefined
# SYMBOLS:      Value: 0x0
# SYMBOLS:      Binding: Weak
# SYMBOLS:      Section: Undefined
# SYMBOLS:      Name: common_object
# SYMBOLS:      Value: 0x4020
# SYMBOLS:      Type: Object
# SYMBOLS:      Name: absolute_symbol
# SYMBOLS:      Value: 0x1234
# SYMBOLS:      Section: Absolute
# SYMBOLS:      Name: script_symbol
# SYMBOLS:      Value: 0x5000
# SYMBOLS:      Section: Absolute

## The section-symbol relocation resolves to .rodata's final address. The weak
## undefined symbol resolves to zero; aliases and linker-defined symbols use
## their final generic ELF values.
# DATA:      Contents of section .data:
# DATA-NEXT: 3000 00000000 00002000 00000000 00002008
# DATA-NEXT: 3010 00000000 00000000 00000000 00004020
# DATA-NEXT: 3020 00000000 00001234 00000000 00002008
# DATA-NEXT: 3030 00000000 00005000

# SEGMENTS:      LOAD {{.*}} R E
# SEGMENTS-NEXT: LOAD {{.*}} R
# SEGMENTS-NEXT: LOAD {{.*}} RW
# SEGMENTS:      Section to Segment mapping:
# SEGMENTS:      00     .text
# SEGMENTS-NEXT: 01     .rodata
# SEGMENTS-NEXT: 02     .data .orphan .bss
# SEGMENTS:      None   .note.mmix

# DUPLICATE: duplicate symbol: global_object
# DUPLICATE: defined at {{.*}}main.o
# DUPLICATE: defined at {{.*}}duplicate.o
# UNRESOLVED: undefined symbol: missing
# UNRESOLVED: referenced by {{.*}}unresolved.o:(.data+0x0)

#--- layout.lds
ENTRY(_start)
SECTIONS {
  .text 0x1000 : { *(.text) }
  .rodata 0x2000 : { *(.rodata) }
  .data 0x3000 : { *(.data) }
  .bss 0x4000 (NOLOAD) : { *(.bss) *(COMMON) }
  PROVIDE(script_symbol = 0x5000);
}

#--- main.s
.section .text,"ax",@progbits
.p2align 4
.global _start
.type _start,@function
_start:
JMP _start
.size _start, .-_start

.type local_function,@function
local_function:
SWYM 0, 0, 0
.size local_function, .-local_function

.global global_function
.type global_function,@function
global_function:
SWYM 0, 0, 0
.size global_function, .-global_function

.global hidden_function
.hidden hidden_function
.type hidden_function,@function
hidden_function:
SWYM 0, 0, 0
.size hidden_function, .-hidden_function

.weak weak_function
.type weak_function,@function
weak_function:
SWYM 0, 0, 0
.size weak_function, .-weak_function

.section .rodata,"a",@progbits
.p2align 5
local_rodata:
.quad 0x1122334455667788
.global global_object
.type global_object,@object
global_object:
.quad 0x99aabbccddeeff00
.size global_object, .-global_object
.global object_alias
.type object_alias,@object
.set object_alias, global_object

.weak weak_undefined
.comm common_object,8,32
.global absolute_symbol
.set absolute_symbol, 0x1234
.global script_symbol

.section .data,"aw",@progbits
.p2align 5
.quad .rodata
.quad global_object
.quad weak_undefined
.quad common_object
.quad absolute_symbol
.quad object_alias
.quad script_symbol

.section .bss,"aw",@nobits
.p2align 6
.zero 16

.section .orphan,"aw",@progbits
.p2align 4
.quad 0xabcdef

.section .note.mmix,"",@note
.p2align 2
.long 0

#--- duplicate.s
.data
.global global_object
.type global_object,@object
global_object:
.quad 0

#--- unresolved.s
.text
.global unresolved_entry
.type unresolved_entry,@function
unresolved_entry:
JMP unresolved_entry
.data
.quad missing
.global missing
