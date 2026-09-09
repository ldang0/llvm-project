; RUN: llc -mtriple=mmix-unknown-elf -filetype=obj %s -o %t.o
; RUN: llvm-readobj --sections --section-data --relocations %t.o \
; RUN:   | FileCheck %s --check-prefix=SECTIONS
; RUN: llvm-readobj --symbols %t.o \
; RUN:   | FileCheck %s --check-prefix=SYMBOLS

; SECTIONS:      Name: .text
; SECTIONS-NEXT: Type: SHT_PROGBITS
; SECTIONS-NEXT: Flags [
; SECTIONS-NEXT:   SHF_ALLOC
; SECTIONS-NEXT:   SHF_EXECINSTR
; SECTIONS-NEXT: ]
; SECTIONS:      Size: 20
; SECTIONS:      AddressAlignment: 4
; SECTIONS-NEXT: EntrySize: 0
; SECTIONS-NEXT: SectionData (
; SECTIONS-NEXT:   0000: F8000000 F8000000 F8000000 F8000000
; SECTIONS-NEXT:   0010: F8000000
; SECTIONS-NEXT: )

; SECTIONS:      Name: .data
; SECTIONS-NEXT: Type: SHT_PROGBITS
; SECTIONS-NEXT: Flags [
; SECTIONS-NEXT:   SHF_ALLOC
; SECTIONS-NEXT:   SHF_WRITE
; SECTIONS-NEXT: ]
; SECTIONS:      Size: 18
; SECTIONS:      AddressAlignment: 8
; SECTIONS-NEXT: EntrySize: 0
; SECTIONS-NEXT: SectionData (
; SECTIONS-NEXT:   0000: 11223344 55667788 12347F00 05060708
; SECTIONS-NEXT:   0010: 090A
; SECTIONS-NEXT: )

; SECTIONS:      Name: .rodata
; SECTIONS-NEXT: Type: SHT_PROGBITS
; SECTIONS-NEXT: Flags [
; SECTIONS-NEXT:   SHF_ALLOC
; SECTIONS-NEXT: ]
; SECTIONS:      Size: 4
; SECTIONS:      AddressAlignment: 4
; SECTIONS-NEXT: EntrySize: 0
; SECTIONS-NEXT: SectionData (
; SECTIONS-NEXT:   0000: 01020304
; SECTIONS-NEXT: )

; SECTIONS:      Name: .bss
; SECTIONS-NEXT: Type: SHT_NOBITS
; SECTIONS-NEXT: Flags [
; SECTIONS-NEXT:   SHF_ALLOC
; SECTIONS-NEXT:   SHF_WRITE
; SECTIONS-NEXT: ]
; SECTIONS:      Size: 24
; SECTIONS:      AddressAlignment: 32
; SECTIONS-NEXT: EntrySize: 0

; SECTIONS:      Name: .rodata.cst8
; SECTIONS-NEXT: Type: SHT_PROGBITS
; SECTIONS-NEXT: Flags [
; SECTIONS-NEXT:   SHF_ALLOC
; SECTIONS-NEXT:   SHF_MERGE
; SECTIONS-NEXT: ]
; SECTIONS:      Size: 8
; SECTIONS:      AddressAlignment: 8
; SECTIONS-NEXT: EntrySize: 8
; SECTIONS-NEXT: SectionData (
; SECTIONS-NEXT:   0000: 01020304 05060708
; SECTIONS-NEXT: )

; SECTIONS:      Name: .rodata.str1.1
; SECTIONS-NEXT: Type: SHT_PROGBITS
; SECTIONS-NEXT: Flags [
; SECTIONS-NEXT:   SHF_ALLOC
; SECTIONS-NEXT:   SHF_MERGE
; SECTIONS-NEXT:   SHF_STRINGS
; SECTIONS-NEXT: ]
; SECTIONS:      Size: 6
; SECTIONS:      AddressAlignment: 1
; SECTIONS-NEXT: EntrySize: 1
; SECTIONS-NEXT: SectionData (
; SECTIONS-NEXT:   0000: 68656C6C 6F00
; SECTIONS-NEXT: )

; SECTIONS:      Relocations [
; SECTIONS-NEXT: ]

; SYMBOLS:      Symbols [
; SYMBOLS-NEXT:   Symbol {
; SYMBOLS-NEXT:     Name:  (0)
; SYMBOLS-NEXT:     Value: 0x0
; SYMBOLS-NEXT:     Size: 0
; SYMBOLS-NEXT:     Binding: Local
; SYMBOLS-NEXT:     Type: None
; SYMBOLS-NEXT:     Other: 0
; SYMBOLS-NEXT:     Section: Undefined
; SYMBOLS-NEXT:   }
; SYMBOLS-NEXT:   Symbol {
; SYMBOLS-NEXT:     Name: elf-object-sections.ll
; SYMBOLS-NEXT:     Value: 0x0
; SYMBOLS-NEXT:     Size: 0
; SYMBOLS-NEXT:     Binding: Local
; SYMBOLS-NEXT:     Type: File
; SYMBOLS-NEXT:     Other: 0
; SYMBOLS-NEXT:     Section: Absolute
; SYMBOLS-NEXT:   }
; SYMBOLS-NEXT:   Symbol {
; SYMBOLS-NEXT:     Name: local_function
; SYMBOLS-NEXT:     Value: 0x0
; SYMBOLS-NEXT:     Size: 4
; SYMBOLS-NEXT:     Binding: Local
; SYMBOLS-NEXT:     Type: Function
; SYMBOLS-NEXT:     Other: 0
; SYMBOLS-NEXT:     Section: .text
; SYMBOLS-NEXT:   }
; SYMBOLS-NEXT:   Symbol {
; SYMBOLS-NEXT:     Name: local_data
; SYMBOLS-NEXT:     Value: 0x8
; SYMBOLS-NEXT:     Size: 2
; SYMBOLS-NEXT:     Binding: Local
; SYMBOLS-NEXT:     Type: Object
; SYMBOLS-NEXT:     Other: 0
; SYMBOLS-NEXT:     Section: .data
; SYMBOLS-NEXT:   }

; SYMBOLS:      Name: weak_function
; SYMBOLS-NEXT: Value: 0x4
; SYMBOLS-NEXT: Size: 4
; SYMBOLS-NEXT: Binding: Weak
; SYMBOLS-NEXT: Type: Function
; SYMBOLS-NEXT: Other: 0
; SYMBOLS-NEXT: Section: .text
; SYMBOLS:      Name: hidden_function
; SYMBOLS-NEXT: Value: 0x8
; SYMBOLS-NEXT: Size: 4
; SYMBOLS-NEXT: Binding: Global
; SYMBOLS-NEXT: Type: Function
; SYMBOLS-NEXT: Other [
; SYMBOLS-NEXT:   STV_HIDDEN
; SYMBOLS-NEXT: ]
; SYMBOLS-NEXT: Section: .text
; SYMBOLS:      Name: protected_function
; SYMBOLS-NEXT: Value: 0xC
; SYMBOLS-NEXT: Size: 4
; SYMBOLS-NEXT: Binding: Global
; SYMBOLS-NEXT: Type: Function
; SYMBOLS-NEXT: Other [
; SYMBOLS-NEXT:   STV_PROTECTED
; SYMBOLS-NEXT: ]
; SYMBOLS-NEXT: Section: .text
; SYMBOLS:      Name: global_function
; SYMBOLS-NEXT: Value: 0x10
; SYMBOLS-NEXT: Size: 4
; SYMBOLS-NEXT: Binding: Global
; SYMBOLS-NEXT: Type: Function
; SYMBOLS-NEXT: Other: 0
; SYMBOLS-NEXT: Section: .text

; SYMBOLS:      Name: writable_data
; SYMBOLS-NEXT: Value: 0x0
; SYMBOLS-NEXT: Size: 8
; SYMBOLS-NEXT: Binding: Global
; SYMBOLS-NEXT: Type: Object
; SYMBOLS-NEXT: Other: 0
; SYMBOLS-NEXT: Section: .data
; SYMBOLS:      Name: readonly_data
; SYMBOLS-NEXT: Value: 0x0
; SYMBOLS-NEXT: Size: 4
; SYMBOLS-NEXT: Binding: Global
; SYMBOLS-NEXT: Type: Object
; SYMBOLS-NEXT: Other: 0
; SYMBOLS-NEXT: Section: .rodata
; SYMBOLS:      Name: zero_storage
; SYMBOLS-NEXT: Value: 0x0
; SYMBOLS-NEXT: Size: 24
; SYMBOLS-NEXT: Binding: Global
; SYMBOLS-NEXT: Type: Object
; SYMBOLS-NEXT: Other: 0
; SYMBOLS-NEXT: Section: .bss
; SYMBOLS:      Name: weak_data
; SYMBOLS-NEXT: Value: 0xA
; SYMBOLS-NEXT: Size: 1
; SYMBOLS-NEXT: Binding: Weak
; SYMBOLS-NEXT: Type: Object
; SYMBOLS-NEXT: Other: 0
; SYMBOLS-NEXT: Section: .data
; SYMBOLS:      Name: hidden_data
; SYMBOLS-NEXT: Value: 0xC
; SYMBOLS-NEXT: Size: 4
; SYMBOLS-NEXT: Binding: Global
; SYMBOLS-NEXT: Type: Object
; SYMBOLS-NEXT: Other [
; SYMBOLS-NEXT:   STV_HIDDEN
; SYMBOLS-NEXT: ]
; SYMBOLS-NEXT: Section: .data
; SYMBOLS:      Name: protected_data
; SYMBOLS-NEXT: Value: 0x10
; SYMBOLS-NEXT: Size: 2
; SYMBOLS-NEXT: Binding: Global
; SYMBOLS-NEXT: Type: Object
; SYMBOLS-NEXT: Other [
; SYMBOLS-NEXT:   STV_PROTECTED
; SYMBOLS-NEXT: ]
; SYMBOLS-NEXT: Section: .data
; SYMBOLS:      Name: common_data
; SYMBOLS-NEXT: Value: 0x10
; SYMBOLS-NEXT: Size: 16
; SYMBOLS-NEXT: Binding: Global
; SYMBOLS-NEXT: Type: Object
; SYMBOLS-NEXT: Other: 0
; SYMBOLS-NEXT: Section: Common

target triple = "mmix-unknown-elf"

@writable_data = global i64 1234605616436508552, align 8
@readonly_data = constant i32 16909060, align 4
@zero_storage = global [24 x i8] zeroinitializer, align 32
@local_data = internal global i16 4660, align 2
@weak_data = weak global i8 127, align 1
@hidden_data = hidden global i32 84281096, align 4
@protected_data = protected global i16 2314, align 2
@common_data = common global [16 x i8] zeroinitializer, align 16
@mergeable_constant = private unnamed_addr constant i64 72623859790382856,
    align 8
@mergeable_string = private unnamed_addr constant [6 x i8] c"hello\00",
    align 1

define internal void @local_function() nounwind {
  ret void
}

define weak void @weak_function() nounwind {
  ret void
}

define hidden void @hidden_function() nounwind {
  ret void
}

define protected void @protected_function() nounwind {
  ret void
}

define void @global_function() nounwind {
  ret void
}
