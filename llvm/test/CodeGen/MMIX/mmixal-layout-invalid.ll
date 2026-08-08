; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm \
; RUN:   --output-asm-variant=1 %s -o %t 2>&1 | FileCheck %s
; RUN: test ! -s %t

target triple = "mmix-unknown-elf"

@prefix = internal global i8 1, align 1
@too_aligned = internal global i8 2, align 134217728

define void @Main() {
entry:
  br label %loop

loop:
  br label %loop
}

; CHECK: error: MMIXAL layout item 'too_aligned': alignment crosses its allocation window
