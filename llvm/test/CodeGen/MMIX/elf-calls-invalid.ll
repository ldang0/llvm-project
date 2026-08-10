; RUN: split-file %s %t

; RUN: not llc -mtriple=mmix-unknown-elf -filetype=obj \
; RUN:   %t/misaligned.ll -o %t/misaligned.o 2>&1 \
; RUN:   | FileCheck %s --check-prefix=MISALIGNED
; RUN: not test -e %t/misaligned.o

; RUN: not llc -mtriple=mmix-unknown-elf -filetype=obj \
; RUN:   %t/calling-convention.ll -o %t/calling-convention.o 2>&1 \
; RUN:   | FileCheck %s --check-prefix=CALLING-CONVENTION
; RUN: not test -e %t/calling-convention.o

; A malformed local symbol-plus-addend reaches the ordinary direct-call
; assembler diagnostic. It is not reclassified as a request for a linker
; stub merely because the ELF path supports unresolved calls.
; MISALIGNED: error: MMIX direct call target is not instruction aligned

; Unsupported ABI-level call forms are rejected before they can be mistaken
; for a relocation strategy.
; CALLING-CONVENTION: LLVM ERROR: MMIX supports only the C calling convention in function 'owner'

;--- misaligned.ll
target triple = "mmix-unknown-elf"

define internal void @callee() {
  ret void
}

define void @owner() {
  call void getelementptr (i8, ptr @callee, i64 2)()
  ret void
}

;--- calling-convention.ll
target triple = "mmix-unknown-elf"

declare fastcc void @callee()

define void @owner() {
  call fastcc void @callee()
  ret void
}
