; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm \
; RUN:   --output-asm-variant=1 %s -o %t.s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=SOURCE
; RUN: test ! -s %t.s
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=obj \
; RUN:   --output-asm-variant=1 %s -o %t.o 2>&1 \
; RUN:   | FileCheck %s --check-prefix=OBJECT
; RUN: test ! -s %t.o

; SOURCE: error: MMIXAL complete-source emission is not available
; SOURCE-NOT: ADD $1
; OBJECT: error: target does not support generation of this file type

target triple = "mmix-unknown-elf"

define i64 @complete_source_guard(i64 %lhs, i64 %rhs) {
  %sum = add i64 %lhs, %rhs
  ret i64 %sum
}
