# RUN: llvm-mc -triple=mmix -filetype=obj %s -o %t
# RUN: llvm-readobj --sections --section-data --relocations %t \
# RUN:   | FileCheck %s --implicit-check-not='Name: .rel' \
# RUN:       --implicit-check-not='Name: .rela'

.text
SWYM 0, 0, 0
.p2align 3
.long 0x11223344

.section .rodata,"a",@progbits
.byte 0xaa
.p2align 2
.long 0xbbccddee

.data
.byte 0x01
.p2align 1
.short 0x2345
.p2align 2
.long 0x6789abcd
.p2align 3
.quad 0xef0123456789abcd

.bss
.zero 3
.p2align 3
.zero 8

.section .rodata.cst8,"aM",@progbits,8
.p2align 3
.quad 0x0123456789abcdef

.section .comment,"",@progbits
.asciz "MMIX"

.section .GCC.command.line,"MS",@progbits,1
.asciz "mmix-test"

.section .llvm_stats,"",@progbits
.byte 0x5a

# CHECK:      Name: .text
# CHECK-NEXT: Type: SHT_PROGBITS (0x1)
# CHECK-NEXT: Flags [ (0x6)
# CHECK-NEXT:   SHF_ALLOC (0x2)
# CHECK-NEXT:   SHF_EXECINSTR (0x4)
# CHECK-NEXT: ]
# CHECK-NEXT: Address: 0x0
# CHECK-NEXT: Offset: 0x40
# CHECK-NEXT: Size: 12
# CHECK-NEXT: Link: 0
# CHECK-NEXT: Info: 0
# CHECK-NEXT: AddressAlignment: 8
# CHECK-NEXT: EntrySize: 0
# CHECK-NEXT: SectionData (
# CHECK-NEXT:   0000: FD000000 00000000 11223344
# CHECK-NEXT: )

# CHECK:      Name: .rodata
# CHECK-NEXT: Type: SHT_PROGBITS (0x1)
# CHECK-NEXT: Flags [ (0x2)
# CHECK-NEXT:   SHF_ALLOC (0x2)
# CHECK-NEXT: ]
# CHECK-NEXT: Address: 0x0
# CHECK-NEXT: Offset: 0x4C
# CHECK-NEXT: Size: 8
# CHECK-NEXT: Link: 0
# CHECK-NEXT: Info: 0
# CHECK-NEXT: AddressAlignment: 4
# CHECK-NEXT: EntrySize: 0
# CHECK-NEXT: SectionData (
# CHECK-NEXT:   0000: AA000000 BBCCDDEE
# CHECK-NEXT: )

# CHECK:      Name: .data
# CHECK-NEXT: Type: SHT_PROGBITS (0x1)
# CHECK-NEXT: Flags [ (0x3)
# CHECK-NEXT:   SHF_ALLOC (0x2)
# CHECK-NEXT:   SHF_WRITE (0x1)
# CHECK-NEXT: ]
# CHECK-NEXT: Address: 0x0
# CHECK-NEXT: Offset: 0x58
# CHECK-NEXT: Size: 16
# CHECK-NEXT: Link: 0
# CHECK-NEXT: Info: 0
# CHECK-NEXT: AddressAlignment: 8
# CHECK-NEXT: EntrySize: 0
# CHECK-NEXT: SectionData (
# CHECK-NEXT:   0000: 01002345 6789ABCD EF012345 6789ABCD
# CHECK-NEXT: )

# CHECK:      Name: .bss
# CHECK-NEXT: Type: SHT_NOBITS (0x8)
# CHECK-NEXT: Flags [ (0x3)
# CHECK-NEXT:   SHF_ALLOC (0x2)
# CHECK-NEXT:   SHF_WRITE (0x1)
# CHECK-NEXT: ]
# CHECK-NEXT: Address: 0x0
# CHECK-NEXT: Offset: 0x68
# CHECK-NEXT: Size: 16
# CHECK-NEXT: Link: 0
# CHECK-NEXT: Info: 0
# CHECK-NEXT: AddressAlignment: 8
# CHECK-NEXT: EntrySize: 0
# CHECK-NEXT: }

# CHECK:      Name: .rodata.cst8
# CHECK-NEXT: Type: SHT_PROGBITS (0x1)
# CHECK-NEXT: Flags [ (0x12)
# CHECK-NEXT:   SHF_ALLOC (0x2)
# CHECK-NEXT:   SHF_MERGE (0x10)
# CHECK-NEXT: ]
# CHECK-NEXT: Address: 0x0
# CHECK-NEXT: Offset: 0x68
# CHECK-NEXT: Size: 8
# CHECK-NEXT: Link: 0
# CHECK-NEXT: Info: 0
# CHECK-NEXT: AddressAlignment: 8
# CHECK-NEXT: EntrySize: 8
# CHECK-NEXT: SectionData (
# CHECK-NEXT:   0000: 01234567 89ABCDEF
# CHECK-NEXT: )

# CHECK:      Name: .comment
# CHECK-NEXT: Type: SHT_PROGBITS (0x1)
# CHECK-NEXT: Flags [ (0x0)
# CHECK-NEXT: ]
# CHECK-NEXT: Address: 0x0
# CHECK-NEXT: Offset: 0x70
# CHECK-NEXT: Size: 5
# CHECK-NEXT: Link: 0
# CHECK-NEXT: Info: 0
# CHECK-NEXT: AddressAlignment: 1
# CHECK-NEXT: EntrySize: 0
# CHECK-NEXT: SectionData (
# CHECK-NEXT:   0000: 4D4D4958 00
# CHECK-NEXT: )

# CHECK:      Name: .GCC.command.line
# CHECK-NEXT: Type: SHT_PROGBITS (0x1)
# CHECK-NEXT: Flags [ (0x30)
# CHECK-NEXT:   SHF_MERGE (0x10)
# CHECK-NEXT:   SHF_STRINGS (0x20)
# CHECK-NEXT: ]
# CHECK-NEXT: Address: 0x0
# CHECK-NEXT: Offset: 0x75
# CHECK-NEXT: Size: 10
# CHECK-NEXT: Link: 0
# CHECK-NEXT: Info: 0
# CHECK-NEXT: AddressAlignment: 1
# CHECK-NEXT: EntrySize: 1
# CHECK-NEXT: SectionData (
# CHECK-NEXT:   0000: 6D6D6978 2D746573 7400
# CHECK-NEXT: )

# CHECK:      Name: .llvm_stats
# CHECK-NEXT: Type: SHT_PROGBITS (0x1)
# CHECK-NEXT: Flags [ (0x0)
# CHECK-NEXT: ]
# CHECK-NEXT: Address: 0x0
# CHECK-NEXT: Offset: 0x7F
# CHECK-NEXT: Size: 1
# CHECK-NEXT: Link: 0
# CHECK-NEXT: Info: 0
# CHECK-NEXT: AddressAlignment: 1
# CHECK-NEXT: EntrySize: 0
# CHECK-NEXT: SectionData (
# CHECK-NEXT:   0000: 5A
# CHECK-NEXT: )

# CHECK:      Relocations [
# CHECK-NEXT: ]
