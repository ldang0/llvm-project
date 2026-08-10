# RUN: llvm-mc -triple=mmix -filetype=obj %s -o %t
# RUN: llvm-readobj --sections --section-data --symbols --relocations \
# RUN:   --expand-relocs %t | FileCheck %s

.section .rodata,"a",@progbits
.byte 0x10
.local local_target
local_target:
.byte 0x11

.global global_target
global_target:
.byte 0x22

.weak weak_defined
weak_defined:
.byte 0x33

.global undefined_global
.weak weak_undefined
.comm common_target,8,8

.set local_absolute, 0x5a
.global global_absolute
.set global_absolute, 0x1234

.data
.byte undefined_global + 1
.p2align 1
.short undefined_global - 2
.p2align 2
.long undefined_global + 3
.p2align 3
.quad undefined_global - 4

# A structure-shaped layout with deliberately unaligned symbolic fields.
.byte 0xa1
.byte local_target + 5
.short global_target - 6
.byte 0xb2
.long undefined_global + 7
.quad weak_defined - 8
.byte weak_undefined + 9
.byte 0xc3
.short .rodata + 10
.long common_target - 11

# Assembly-time absolute values remain in the field and need no relocation.
.byte local_absolute
.short global_absolute
.long 0x89abcdef
.quad 0x0123456789abcdef

.section .data.pcrel,"aw",@progbits
.Lpc_base_8:
.byte undefined_global - .Lpc_base_8 + 1
.Lpc_base_16:
.short undefined_global - .Lpc_base_16 - 2
.Lpc_base_32:
.long undefined_global - .Lpc_base_32 + 3
.Lpc_base_64:
.quad undefined_global - .Lpc_base_64 - 4

# A same-section difference remains assembly-time resolved.
.Lresolved_start:
.byte 0xa5
.Lresolved_end:
.short .Lresolved_end - .Lresolved_start

# CHECK:      Name: .rodata
# CHECK-NEXT: Type: SHT_PROGBITS (0x1)
# CHECK:      Size: 4
# CHECK:      AddressAlignment: 1
# CHECK-NEXT: EntrySize: 0
# CHECK-NEXT: SectionData (
# CHECK-NEXT:   0000: 10112233
# CHECK-NEXT: )

# CHECK:      Index: [[DATA:[0-9]+]]
# CHECK-NEXT: Name: .data
# CHECK-NEXT: Type: SHT_PROGBITS (0x1)
# CHECK:      Size: 56
# CHECK:      AddressAlignment: 8
# CHECK-NEXT: EntrySize: 0
# CHECK-NEXT: SectionData (
# CHECK-NEXT:   0000: 00000000 00000000 00000000 00000000
# CHECK-NEXT:   0010: A1000000 B2000000 00000000 00000000
# CHECK-NEXT:   0020: 0000C300 00000000 005A1234 89ABCDEF
# CHECK-NEXT:   0030: 01234567 89ABCDEF
# CHECK-NEXT: )

# CHECK:      Index: [[RELA:[0-9]+]]
# CHECK-NEXT: Name: .rela.data
# CHECK-NEXT: Type: SHT_RELA (0x4)
# CHECK-NEXT: Flags [ (0x40)
# CHECK-NEXT:   SHF_INFO_LINK (0x40)
# CHECK-NEXT: ]
# CHECK:      Size: 264
# CHECK:      Link: [[SYMTAB:[0-9]+]]
# CHECK-NEXT: Info: [[DATA]]
# CHECK-NEXT: AddressAlignment: 8
# CHECK-NEXT: EntrySize: 24

# CHECK:      Index: [[PCREL_DATA:[0-9]+]]
# CHECK-NEXT: Name: .data.pcrel
# CHECK-NEXT: Type: SHT_PROGBITS (0x1)
# CHECK:      Size: 18
# CHECK:      AddressAlignment: 1
# CHECK-NEXT: EntrySize: 0
# CHECK-NEXT: SectionData (
# CHECK-NEXT:   0000: 00000000 00000000 00000000 000000A5
# CHECK-NEXT:   0010: 0001
# CHECK-NEXT: )

# CHECK:      Index: [[PCREL_RELA:[0-9]+]]
# CHECK-NEXT: Name: .rela.data.pcrel
# CHECK-NEXT: Type: SHT_RELA (0x4)
# CHECK-NEXT: Flags [ (0x40)
# CHECK-NEXT:   SHF_INFO_LINK (0x40)
# CHECK-NEXT: ]
# CHECK:      Size: 96
# CHECK:      Link: [[SYMTAB]]
# CHECK-NEXT: Info: [[PCREL_DATA]]
# CHECK-NEXT: AddressAlignment: 8
# CHECK-NEXT: EntrySize: 24

# CHECK:      Index: [[SYMTAB]]
# CHECK-NEXT: Name: .symtab
# CHECK-NEXT: Type: SHT_SYMTAB (0x2)

# CHECK:      Relocations [
# CHECK-NEXT:   Section ([[RELA]]) .rela.data {
# CHECK-NEXT:     Relocation {
# CHECK-NEXT:       Offset: 0x0
# CHECK-NEXT:       Type: R_MMIX_8 (1)
# CHECK-NEXT:       Symbol: undefined_global ([[UNDEFINED:[0-9]+]])
# CHECK-NEXT:       Addend: 0x1
# CHECK-NEXT:     }
# CHECK-NEXT:     Relocation {
# CHECK-NEXT:       Offset: 0x2
# CHECK-NEXT:       Type: R_MMIX_16 (2)
# CHECK-NEXT:       Symbol: undefined_global ([[UNDEFINED]])
# CHECK-NEXT:       Addend: 0xFFFFFFFFFFFFFFFE
# CHECK-NEXT:     }
# CHECK-NEXT:     Relocation {
# CHECK-NEXT:       Offset: 0x4
# CHECK-NEXT:       Type: R_MMIX_32 (4)
# CHECK-NEXT:       Symbol: undefined_global ([[UNDEFINED]])
# CHECK-NEXT:       Addend: 0x3
# CHECK-NEXT:     }
# CHECK-NEXT:     Relocation {
# CHECK-NEXT:       Offset: 0x8
# CHECK-NEXT:       Type: R_MMIX_64 (5)
# CHECK-NEXT:       Symbol: undefined_global ([[UNDEFINED]])
# CHECK-NEXT:       Addend: 0xFFFFFFFFFFFFFFFC
# CHECK-NEXT:     }
# CHECK-NEXT:     Relocation {
# CHECK-NEXT:       Offset: 0x11
# CHECK-NEXT:       Type: R_MMIX_8 (1)
# CHECK-NEXT:       Symbol: .rodata ([[RODATA:[0-9]+]])
# CHECK-NEXT:       Addend: 0x6
# CHECK-NEXT:     }
# CHECK-NEXT:     Relocation {
# CHECK-NEXT:       Offset: 0x12
# CHECK-NEXT:       Type: R_MMIX_16 (2)
# CHECK-NEXT:       Symbol: global_target ([[GLOBAL:[0-9]+]])
# CHECK-NEXT:       Addend: 0xFFFFFFFFFFFFFFFA
# CHECK-NEXT:     }
# CHECK-NEXT:     Relocation {
# CHECK-NEXT:       Offset: 0x15
# CHECK-NEXT:       Type: R_MMIX_32 (4)
# CHECK-NEXT:       Symbol: undefined_global ([[UNDEFINED]])
# CHECK-NEXT:       Addend: 0x7
# CHECK-NEXT:     }
# CHECK-NEXT:     Relocation {
# CHECK-NEXT:       Offset: 0x19
# CHECK-NEXT:       Type: R_MMIX_64 (5)
# CHECK-NEXT:       Symbol: weak_defined ([[WEAK_DEFINED:[0-9]+]])
# CHECK-NEXT:       Addend: 0xFFFFFFFFFFFFFFF8
# CHECK-NEXT:     }
# CHECK-NEXT:     Relocation {
# CHECK-NEXT:       Offset: 0x21
# CHECK-NEXT:       Type: R_MMIX_8 (1)
# CHECK-NEXT:       Symbol: weak_undefined ([[WEAK_UNDEFINED:[0-9]+]])
# CHECK-NEXT:       Addend: 0x9
# CHECK-NEXT:     }
# CHECK-NEXT:     Relocation {
# CHECK-NEXT:       Offset: 0x23
# CHECK-NEXT:       Type: R_MMIX_16 (2)
# CHECK-NEXT:       Symbol: .rodata ([[RODATA]])
# CHECK-NEXT:       Addend: 0xA
# CHECK-NEXT:     }
# CHECK-NEXT:     Relocation {
# CHECK-NEXT:       Offset: 0x25
# CHECK-NEXT:       Type: R_MMIX_32 (4)
# CHECK-NEXT:       Symbol: common_target ([[COMMON:[0-9]+]])
# CHECK-NEXT:       Addend: 0xFFFFFFFFFFFFFFF5
# CHECK-NEXT:     }
# CHECK-NEXT:   }
# CHECK-NEXT:   Section ([[PCREL_RELA]]) .rela.data.pcrel {
# CHECK-NEXT:     Relocation {
# CHECK-NEXT:       Offset: 0x0
# CHECK-NEXT:       Type: R_MMIX_PC_8 (6)
# CHECK-NEXT:       Symbol: undefined_global ([[UNDEFINED]])
# CHECK-NEXT:       Addend: 0x1
# CHECK-NEXT:     }
# CHECK-NEXT:     Relocation {
# CHECK-NEXT:       Offset: 0x1
# CHECK-NEXT:       Type: R_MMIX_PC_16 (7)
# CHECK-NEXT:       Symbol: undefined_global ([[UNDEFINED]])
# CHECK-NEXT:       Addend: 0xFFFFFFFFFFFFFFFE
# CHECK-NEXT:     }
# CHECK-NEXT:     Relocation {
# CHECK-NEXT:       Offset: 0x3
# CHECK-NEXT:       Type: R_MMIX_PC_32 (9)
# CHECK-NEXT:       Symbol: undefined_global ([[UNDEFINED]])
# CHECK-NEXT:       Addend: 0x3
# CHECK-NEXT:     }
# CHECK-NEXT:     Relocation {
# CHECK-NEXT:       Offset: 0x7
# CHECK-NEXT:       Type: R_MMIX_PC_64 (10)
# CHECK-NEXT:       Symbol: undefined_global ([[UNDEFINED]])
# CHECK-NEXT:       Addend: 0xFFFFFFFFFFFFFFFC
# CHECK-NEXT:     }
# CHECK-NEXT:   }
# CHECK-NEXT: ]

# CHECK:      Symbols [
# CHECK:      Name: .rodata
# CHECK-NEXT: Value: 0x0
# CHECK-NEXT: Size: 0
# CHECK-NEXT: Binding: Local (0x0)
# CHECK-NEXT: Type: Section (0x3)
# CHECK-NEXT: Other: 0
# CHECK-NEXT: Section: .rodata
# CHECK:      Name: local_target
# CHECK-NEXT: Value: 0x1
# CHECK-NEXT: Size: 0
# CHECK-NEXT: Binding: Local (0x0)
# CHECK-NEXT: Type: None (0x0)
# CHECK-NEXT: Other: 0
# CHECK-NEXT: Section: .rodata
# CHECK:      Name: local_absolute
# CHECK-NEXT: Value: 0x5A
# CHECK-NEXT: Size: 0
# CHECK-NEXT: Binding: Local (0x0)
# CHECK-NEXT: Type: None (0x0)
# CHECK-NEXT: Other: 0
# CHECK-NEXT: Section: Absolute (0xFFF1)
# CHECK:      Name: global_target
# CHECK-NEXT: Value: 0x2
# CHECK-NEXT: Size: 0
# CHECK-NEXT: Binding: Global (0x1)
# CHECK-NEXT: Type: None (0x0)
# CHECK-NEXT: Other: 0
# CHECK-NEXT: Section: .rodata
# CHECK:      Name: weak_defined
# CHECK-NEXT: Value: 0x3
# CHECK-NEXT: Size: 0
# CHECK-NEXT: Binding: Weak (0x2)
# CHECK-NEXT: Type: None (0x0)
# CHECK-NEXT: Other: 0
# CHECK-NEXT: Section: .rodata
# CHECK:      Name: undefined_global
# CHECK-NEXT: Value: 0x0
# CHECK-NEXT: Size: 0
# CHECK-NEXT: Binding: Global (0x1)
# CHECK-NEXT: Type: None (0x0)
# CHECK-NEXT: Other: 0
# CHECK-NEXT: Section: Undefined (0x0)
# CHECK:      Name: weak_undefined
# CHECK-NEXT: Value: 0x0
# CHECK-NEXT: Size: 0
# CHECK-NEXT: Binding: Weak (0x2)
# CHECK-NEXT: Type: None (0x0)
# CHECK-NEXT: Other: 0
# CHECK-NEXT: Section: Undefined (0x0)
# CHECK:      Name: common_target
# CHECK-NEXT: Value: 0x8
# CHECK-NEXT: Size: 8
# CHECK-NEXT: Binding: Global (0x1)
# CHECK-NEXT: Type: Object (0x1)
# CHECK-NEXT: Other: 0
# CHECK-NEXT: Section: Common (0xFFF2)
# CHECK:      Name: global_absolute
# CHECK-NEXT: Value: 0x1234
# CHECK-NEXT: Size: 0
# CHECK-NEXT: Binding: Global (0x1)
# CHECK-NEXT: Type: None (0x0)
# CHECK-NEXT: Other: 0
# CHECK-NEXT: Section: Absolute (0xFFF1)
