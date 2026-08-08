; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm \
; RUN:   --output-asm-variant=1 %s -o %t 2>&1 | FileCheck %s
; RUN: test ! -s %t

target triple = "mmix-unknown-elf"

define void @Main() {
  ret void
}

; CHECK: error: MMIXAL bare-metal entry 'Main' must not have a reachable return
