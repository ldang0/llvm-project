; RUN: llc -mtriple=mmix-unknown-elf -filetype=obj %s -o %t.o
; RUN: llvm-readobj --file-headers --sections --relocations --symbols %t.o \
; RUN:   | FileCheck %s

; CHECK:      Format: elf64-mmix
; CHECK-NEXT: Arch: mmix
; CHECK-NEXT: AddressSize: 64bit
; CHECK:      Class: 64-bit
; CHECK-NEXT: DataEncoding: BigEndian
; CHECK:      Type: Relocatable
; CHECK-NEXT: Machine: EM_MMIX
; CHECK:      ProgramHeaderCount: 0
; CHECK:      Name: .text
; CHECK-NEXT: Type: SHT_PROGBITS
; CHECK:      SHF_ALLOC
; CHECK-NEXT: SHF_EXECINSTR
; CHECK:      AddressAlignment: 4
; CHECK:      Relocations [
; CHECK-NEXT: ]
; CHECK:      Name: smoke
; CHECK:      Size: 4
; CHECK-NEXT: Binding: Global
; CHECK-NEXT: Type: Function
; CHECK:      Section: .text

target triple = "mmix-unknown-elf"

define void @smoke() nounwind {
  ret void
}
