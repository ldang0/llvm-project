; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm \
; RUN:   --output-asm-variant=1 %s -o %t 2>&1 | FileCheck %s
; RUN: test ! -s %t

target triple = "mmix-unknown-elf"

define void @ordinary() {
  ret void
}

; CHECK: error: MMIXAL bare-metal module has no entry named 'Main'
