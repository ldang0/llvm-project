; RUN: llc -mtriple=mmix -O0 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=mmix -O2 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=mmix -filetype=obj < %s -o %t.o
; RUN: llvm-readobj -S -r -x .text %t.o | FileCheck %s --check-prefix=OBJECT

define void @trap() nounwind {
; CHECK-LABEL: trap:
; CHECK:       TRAP 255, 0, 0
; CHECK-NOT:   POP
  call void @llvm.trap()
  unreachable
}

; OBJECT:      Name: .text
; OBJECT-NEXT: Type: SHT_PROGBITS
; OBJECT:      Size: 4
; OBJECT:      Relocations [
; OBJECT-NEXT: ]
; OBJECT:      Hex dump of section '.text':
; OBJECT-NEXT: 0x00000000 00ff0000

declare void @llvm.trap()
