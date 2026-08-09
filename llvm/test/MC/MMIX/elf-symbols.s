# RUN: llvm-mc -triple=mmix -filetype=obj %s -o %t
# RUN: llvm-readobj --sections --section-data --symbols --relocations %t \
# RUN:   | FileCheck %s --implicit-check-not='Name: .rel' \
# RUN:       --implicit-check-not='Name: .rela'

.file "elf-symbols.s"

.text
local_label:
SWYM 0, 0, 0

.local local_function
.type local_function,@function
local_function:
SWYM 0, 0, 0
.size local_function, .-local_function

.global global_function
.hidden global_function
.type global_function,@function
global_function:
SWYM 0, 0, 0
.size global_function, .-global_function

.weak weak_function
.protected weak_function
.type weak_function,@function
weak_function:
SWYM 0, 0, 0
.size weak_function, .-weak_function

.data
.local local_object
.type local_object,@object
local_object:
.quad 0x0123456789abcdef
.size local_object, .-local_object

.global global_object
.type global_object,@object
global_object:
.long 0x11223344
.size global_object, .-global_object

.bss
.p2align 3
.weak weak_object
.type weak_object,@object
weak_object:
.zero 8
.size weak_object, .-weak_object

.global undefined_object
.type undefined_object,@object

.weak weak_undefined
.type weak_undefined,@function

.set local_absolute, 0x123456789abcdef0

.global global_absolute
.set global_absolute, 42

.comm common_object,24,16

.weak weak_common
.comm weak_common,8,8

.addrsig
.addrsig_sym .text
.addrsig_sym .data
.addrsig_sym .bss

# CHECK:      Index: 1
# CHECK-NEXT: Name: .strtab
# CHECK-NEXT: Type: SHT_STRTAB (0x3)
# CHECK-NEXT: Flags [ (0x0)
# CHECK-NEXT: ]
# CHECK-NEXT: Address: 0x0
# CHECK-NEXT: Offset: 0x{{[0-9A-F]+}}
# CHECK-NEXT: Size: {{[1-9][0-9]*}}
# CHECK-NEXT: Link: 0
# CHECK-NEXT: Info: 0
# CHECK-NEXT: AddressAlignment: 1
# CHECK-NEXT: EntrySize: 0
# CHECK-NEXT: SectionData (
# CHECK-NEXT:   0000: 00

# CHECK:      Index: 5
# CHECK-NEXT: Name: .llvm_addrsig
# CHECK-NEXT: Type: SHT_LLVM_ADDRSIG (0x6FFF4C03)
# CHECK-NEXT: Flags [ (0x80000000)
# CHECK-NEXT:   SHF_EXCLUDE (0x80000000)
# CHECK-NEXT: ]
# CHECK-NEXT: Address: 0x0
# CHECK-NEXT: Offset: 0x{{[0-9A-F]+}}
# CHECK-NEXT: Size: 3
# CHECK-NEXT: Link: [[SYMTAB:[0-9]+]]
# CHECK-NEXT: Info: 0
# CHECK-NEXT: AddressAlignment: 1
# CHECK-NEXT: EntrySize: 0
# CHECK-NEXT: SectionData (
# CHECK-NEXT:   0000: 020507
# CHECK-NEXT: )

# CHECK:      Index: [[SYMTAB]]
# CHECK-NEXT: Name: .symtab
# CHECK-NEXT: Type: SHT_SYMTAB (0x2)
# CHECK-NEXT: Flags [ (0x0)
# CHECK-NEXT: ]
# CHECK-NEXT: Address: 0x0
# CHECK-NEXT: Offset: 0x{{[0-9A-F]+}}
# CHECK-NEXT: Size: 432
# CHECK-NEXT: Link: 1
# CHECK-NEXT: Info: 9
# CHECK-NEXT: AddressAlignment: 8
# CHECK-NEXT: EntrySize: 24

# CHECK:      Relocations [
# CHECK-NEXT: ]

# CHECK:      Symbols [
# CHECK-NEXT:   Symbol {
# CHECK-NEXT:     Name:  (0)
# CHECK-NEXT:     Value: 0x0
# CHECK-NEXT:     Size: 0
# CHECK-NEXT:     Binding: Local (0x0)
# CHECK-NEXT:     Type: None (0x0)
# CHECK-NEXT:     Other: 0
# CHECK-NEXT:     Section: Undefined (0x0)
# CHECK-NEXT:   }
# CHECK-NEXT:   Symbol {
# CHECK-NEXT:     Name: elf-symbols.s
# CHECK-NEXT:     Value: 0x0
# CHECK-NEXT:     Size: 0
# CHECK-NEXT:     Binding: Local (0x0)
# CHECK-NEXT:     Type: File (0x4)
# CHECK-NEXT:     Other: 0
# CHECK-NEXT:     Section: Absolute (0xFFF1)
# CHECK-NEXT:   }
# CHECK-NEXT:   Symbol {
# CHECK-NEXT:     Name: .text (0)
# CHECK-NEXT:     Value: 0x0
# CHECK-NEXT:     Size: 0
# CHECK-NEXT:     Binding: Local (0x0)
# CHECK-NEXT:     Type: Section (0x3)
# CHECK-NEXT:     Other: 0
# CHECK-NEXT:     Section: .text (0x2)
# CHECK-NEXT:   }
# CHECK-NEXT:   Symbol {
# CHECK-NEXT:     Name: local_label
# CHECK-NEXT:     Value: 0x0
# CHECK-NEXT:     Size: 0
# CHECK-NEXT:     Binding: Local (0x0)
# CHECK-NEXT:     Type: None (0x0)
# CHECK-NEXT:     Other: 0
# CHECK-NEXT:     Section: .text (0x2)
# CHECK-NEXT:   }
# CHECK-NEXT:   Symbol {
# CHECK-NEXT:     Name: local_function
# CHECK-NEXT:     Value: 0x4
# CHECK-NEXT:     Size: 4
# CHECK-NEXT:     Binding: Local (0x0)
# CHECK-NEXT:     Type: Function (0x2)
# CHECK-NEXT:     Other: 0
# CHECK-NEXT:     Section: .text (0x2)
# CHECK-NEXT:   }
# CHECK-NEXT:   Symbol {
# CHECK-NEXT:     Name: .data (0)
# CHECK-NEXT:     Value: 0x0
# CHECK-NEXT:     Size: 0
# CHECK-NEXT:     Binding: Local (0x0)
# CHECK-NEXT:     Type: Section (0x3)
# CHECK-NEXT:     Other: 0
# CHECK-NEXT:     Section: .data (0x3)
# CHECK-NEXT:   }
# CHECK-NEXT:   Symbol {
# CHECK-NEXT:     Name: local_object
# CHECK-NEXT:     Value: 0x0
# CHECK-NEXT:     Size: 8
# CHECK-NEXT:     Binding: Local (0x0)
# CHECK-NEXT:     Type: Object (0x1)
# CHECK-NEXT:     Other: 0
# CHECK-NEXT:     Section: .data (0x3)
# CHECK-NEXT:   }
# CHECK-NEXT:   Symbol {
# CHECK-NEXT:     Name: .bss (0)
# CHECK-NEXT:     Value: 0x0
# CHECK-NEXT:     Size: 0
# CHECK-NEXT:     Binding: Local (0x0)
# CHECK-NEXT:     Type: Section (0x3)
# CHECK-NEXT:     Other: 0
# CHECK-NEXT:     Section: .bss (0x4)
# CHECK-NEXT:   }
# CHECK-NEXT:   Symbol {
# CHECK-NEXT:     Name: local_absolute
# CHECK-NEXT:     Value: 0x123456789ABCDEF0
# CHECK-NEXT:     Size: 0
# CHECK-NEXT:     Binding: Local (0x0)
# CHECK-NEXT:     Type: None (0x0)
# CHECK-NEXT:     Other: 0
# CHECK-NEXT:     Section: Absolute (0xFFF1)
# CHECK-NEXT:   }
# CHECK-NEXT:   Symbol {
# CHECK-NEXT:     Name: global_function
# CHECK-NEXT:     Value: 0x8
# CHECK-NEXT:     Size: 4
# CHECK-NEXT:     Binding: Global (0x1)
# CHECK-NEXT:     Type: Function (0x2)
# CHECK-NEXT:     Other [ (0x2)
# CHECK-NEXT:       STV_HIDDEN (0x2)
# CHECK-NEXT:     ]
# CHECK-NEXT:     Section: .text (0x2)
# CHECK-NEXT:   }
# CHECK-NEXT:   Symbol {
# CHECK-NEXT:     Name: weak_function
# CHECK-NEXT:     Value: 0xC
# CHECK-NEXT:     Size: 4
# CHECK-NEXT:     Binding: Weak (0x2)
# CHECK-NEXT:     Type: Function (0x2)
# CHECK-NEXT:     Other [ (0x3)
# CHECK-NEXT:       STV_PROTECTED (0x3)
# CHECK-NEXT:     ]
# CHECK-NEXT:     Section: .text (0x2)
# CHECK-NEXT:   }
# CHECK-NEXT:   Symbol {
# CHECK-NEXT:     Name: global_object
# CHECK-NEXT:     Value: 0x8
# CHECK-NEXT:     Size: 4
# CHECK-NEXT:     Binding: Global (0x1)
# CHECK-NEXT:     Type: Object (0x1)
# CHECK-NEXT:     Other: 0
# CHECK-NEXT:     Section: .data (0x3)
# CHECK-NEXT:   }
# CHECK-NEXT:   Symbol {
# CHECK-NEXT:     Name: weak_object
# CHECK-NEXT:     Value: 0x0
# CHECK-NEXT:     Size: 8
# CHECK-NEXT:     Binding: Weak (0x2)
# CHECK-NEXT:     Type: Object (0x1)
# CHECK-NEXT:     Other: 0
# CHECK-NEXT:     Section: .bss (0x4)
# CHECK-NEXT:   }
# CHECK-NEXT:   Symbol {
# CHECK-NEXT:     Name: undefined_object
# CHECK-NEXT:     Value: 0x0
# CHECK-NEXT:     Size: 0
# CHECK-NEXT:     Binding: Global (0x1)
# CHECK-NEXT:     Type: Object (0x1)
# CHECK-NEXT:     Other: 0
# CHECK-NEXT:     Section: Undefined (0x0)
# CHECK-NEXT:   }
# CHECK-NEXT:   Symbol {
# CHECK-NEXT:     Name: weak_undefined
# CHECK-NEXT:     Value: 0x0
# CHECK-NEXT:     Size: 0
# CHECK-NEXT:     Binding: Weak (0x2)
# CHECK-NEXT:     Type: Function (0x2)
# CHECK-NEXT:     Other: 0
# CHECK-NEXT:     Section: Undefined (0x0)
# CHECK-NEXT:   }
# CHECK-NEXT:   Symbol {
# CHECK-NEXT:     Name: global_absolute
# CHECK-NEXT:     Value: 0x2A
# CHECK-NEXT:     Size: 0
# CHECK-NEXT:     Binding: Global (0x1)
# CHECK-NEXT:     Type: None (0x0)
# CHECK-NEXT:     Other: 0
# CHECK-NEXT:     Section: Absolute (0xFFF1)
# CHECK-NEXT:   }
# CHECK-NEXT:   Symbol {
# CHECK-NEXT:     Name: common_object
# CHECK-NEXT:     Value: 0x10
# CHECK-NEXT:     Size: 24
# CHECK-NEXT:     Binding: Global (0x1)
# CHECK-NEXT:     Type: Object (0x1)
# CHECK-NEXT:     Other: 0
# CHECK-NEXT:     Section: Common (0xFFF2)
# CHECK-NEXT:   }
# CHECK-NEXT:   Symbol {
# CHECK-NEXT:     Name: weak_common
# CHECK-NEXT:     Value: 0x8
# CHECK-NEXT:     Size: 8
# CHECK-NEXT:     Binding: Weak (0x2)
# CHECK-NEXT:     Type: Object (0x1)
# CHECK-NEXT:     Other: 0
# CHECK-NEXT:     Section: Common (0xFFF2)
# CHECK-NEXT:   }
# CHECK-NEXT: ]
