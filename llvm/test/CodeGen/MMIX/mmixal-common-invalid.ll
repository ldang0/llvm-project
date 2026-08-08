; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm \
; RUN:   --output-asm-variant=1 %s -o %t 2>&1 | FileCheck %s
; RUN: test ! -s %t

target triple = "mmix-unknown-elf"

@common_data = common global i64 0, align 8

; CHECK: error: MMIXAL does not support common-symbol allocation events
