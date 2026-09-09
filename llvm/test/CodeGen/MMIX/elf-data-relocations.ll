; RUN: llc -mtriple=mmix-unknown-elf -filetype=obj %s -o %t.o
; RUN: llvm-readobj --sections --section-data %t.o \
; RUN:   | FileCheck %s --check-prefix=DATA
; RUN: llvm-readobj --relocations --expand-relocs %t.o \
; RUN:   | FileCheck %s --check-prefix=RELOCS --implicit-check-not=R_MMIX_
; RUN: llvm-readobj --symbols %t.o \
; RUN:   | FileCheck %s --check-prefix=SYMBOLS

; DATA:      Name: .data
; DATA-NEXT: Type: SHT_PROGBITS
; DATA-NEXT: Flags [
; DATA-NEXT:   SHF_ALLOC
; DATA-NEXT:   SHF_WRITE
; DATA-NEXT: ]
; DATA:      Size: 40
; DATA:      AddressAlignment: 8
; DATA-NEXT: EntrySize: 0
; DATA-NEXT: SectionData (
; DATA-NEXT:   0000: 00000000 00000000 00000000 00000000
; DATA-NEXT:   0010: 00000000 00000000 00000000 00000000
; DATA-NEXT:   0020: 00000000 00000000
; DATA-NEXT: )
; DATA:      Name: .rela.data
; DATA-NEXT: Type: SHT_RELA
; DATA:      Size: 168
; DATA:      AddressAlignment: 8
; DATA-NEXT: EntrySize: 24

; RELOCS:      Section {{.*}} .rela.data {
; RELOCS-NEXT:   Relocation {
; RELOCS-NEXT:     Offset: 0x0
; RELOCS-NEXT:     Type: R_MMIX_8 (1)
; RELOCS-NEXT:     Symbol: defined_data
; RELOCS-NEXT:     Addend: 0x0
; RELOCS-NEXT:   }
; RELOCS-NEXT:   Relocation {
; RELOCS-NEXT:     Offset: 0x2
; RELOCS-NEXT:     Type: R_MMIX_16 (2)
; RELOCS-NEXT:     Symbol: external_data
; RELOCS-NEXT:     Addend: 0x0
; RELOCS-NEXT:   }
; RELOCS-NEXT:   Relocation {
; RELOCS-NEXT:     Offset: 0x4
; RELOCS-NEXT:     Type: R_MMIX_32 (4)
; RELOCS-NEXT:     Symbol: external_function
; RELOCS-NEXT:     Addend: 0x0
; RELOCS-NEXT:   }
; RELOCS-NEXT:   Relocation {
; RELOCS-NEXT:     Offset: 0x8
; RELOCS-NEXT:     Type: R_MMIX_64 (5)
; RELOCS-NEXT:     Symbol: .text
; RELOCS-NEXT:     Addend: 0x0
; RELOCS-NEXT:   }
; RELOCS-NEXT:   Relocation {
; RELOCS-NEXT:     Offset: 0x10
; RELOCS-NEXT:     Type: R_MMIX_64 (5)
; RELOCS-NEXT:     Symbol: .bss
; RELOCS-NEXT:     Addend: 0x8
; RELOCS-NEXT:   }
; RELOCS-NEXT:   Relocation {
; RELOCS-NEXT:     Offset: 0x18
; RELOCS-NEXT:     Type: R_MMIX_64 (5)
; RELOCS-NEXT:     Symbol: external_data
; RELOCS-NEXT:     Addend: 0x1234
; RELOCS-NEXT:   }
; RELOCS-NEXT:   Relocation {
; RELOCS-NEXT:     Offset: 0x20
; RELOCS-NEXT:     Type: R_MMIX_64 (5)
; RELOCS-NEXT:     Symbol: external_data
; RELOCS-NEXT:     Addend: 0xFFFFFFFFFFFFEDCC
; RELOCS-NEXT:   }
; RELOCS-NEXT: }

; SYMBOLS:      Name: .text
; SYMBOLS-NEXT: Value: 0x0
; SYMBOLS-NEXT: Size: 0
; SYMBOLS-NEXT: Binding: Local
; SYMBOLS-NEXT: Type: Section
; SYMBOLS:      Section: .text
; SYMBOLS:      Name: .bss
; SYMBOLS-NEXT: Value: 0x0
; SYMBOLS-NEXT: Size: 0
; SYMBOLS-NEXT: Binding: Local
; SYMBOLS-NEXT: Type: Section
; SYMBOLS:      Section: .bss
; SYMBOLS:      Name: defined_data
; SYMBOLS:      Size: 8
; SYMBOLS-NEXT: Binding: Global
; SYMBOLS-NEXT: Type: Object
; SYMBOLS:      Section: .bss
; SYMBOLS:      Name: external_data
; SYMBOLS-NEXT: Value: 0x0
; SYMBOLS-NEXT: Size: 0
; SYMBOLS-NEXT: Binding: Global
; SYMBOLS-NEXT: Type: None
; SYMBOLS:      Section: Undefined
; SYMBOLS:      Name: external_function (
; SYMBOLS-NEXT: Value: 0x0
; SYMBOLS-NEXT: Size: 0
; SYMBOLS-NEXT: Binding: Global
; SYMBOLS-NEXT: Type: None
; SYMBOLS:      Section: Undefined

target triple = "mmix-unknown-elf"

@defined_data = global i64 0, align 8
@local_data = internal global i8 0, align 1
@external_data = external global i8

@defined_i8 = global i8 ptrtoint (ptr @defined_data to i8), align 1
@external_i16 = global i16 ptrtoint (ptr @external_data to i16), align 2
@external_function_i32 = global i32 ptrtoint (ptr @external_function to i32),
    align 4
@defined_function_i64 = global i64 ptrtoint (ptr @defined_function to i64),
    align 8
@local_i64 = global i64 ptrtoint (ptr @local_data to i64), align 8
@positive_addend = global i64 ptrtoint (
    ptr getelementptr (i8, ptr @external_data, i64 4660) to i64), align 8
@negative_addend = global i64 ptrtoint (
    ptr getelementptr (i8, ptr @external_data, i64 -4660) to i64), align 8

declare void @external_function()

define internal void @defined_function() nounwind {
  ret void
}
