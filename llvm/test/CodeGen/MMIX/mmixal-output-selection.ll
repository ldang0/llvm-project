; RUN: llc -mtriple=mmix-unknown-elf -filetype=asm \
; RUN:   --output-asm-variant=1 %s -o %t.s
; RUN: FileCheck %s --check-prefix=OUTPUT --implicit-check-not=.globl \
; RUN:   --implicit-check-not=.type --implicit-check-not=.size < %t.s
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=obj \
; RUN:   --output-asm-variant=1 %s -o %t.o 2>&1 \
; RUN:   | FileCheck %s --check-prefix=OBJECT
; RUN: test ! -s %t.o

; OBJECT: error: target does not support generation of this file type
; OUTPUT: LOC #0000000000000100
; OUTPUT-NEXT: complete_source_guard	IS @
; OUTPUT: ADDU $

target triple = "mmix-unknown-elf"

define i64 @complete_source_guard(i64 %lhs, i64 %rhs) {
  %sum = add i64 %lhs, %rhs
  ret i64 %sum
}
