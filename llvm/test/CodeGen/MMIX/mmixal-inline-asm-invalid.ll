; RUN: split-file %s %t
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm \
; RUN:   --output-asm-variant=1 %t/module.ll -o %t/module.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=MODULE
; RUN: test ! -s %t/module.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm \
; RUN:   --output-asm-variant=1 %t/empty.ll -o %t/empty.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=EMPTY
; RUN: test ! -s %t/empty.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm \
; RUN:   --output-asm-variant=1 %t/side-effect-free.ll \
; RUN:   -o %t/side-effect-free.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=SIDE-EFFECT-FREE
; RUN: test ! -s %t/side-effect-free.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm \
; RUN:   --output-asm-variant=1 %t/operands.ll -o %t/operands.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=OPERANDS
; RUN: test ! -s %t/operands.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm \
; RUN:   --output-asm-variant=1 %t/asm-goto.ll -o %t/asm-goto.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=ASM-GOTO
; RUN: test ! -s %t/asm-goto.mms

; MODULE: MMIXAL output variant 1 does not support module-level inline assembly
; EMPTY: MMIXAL output variant 1 does not support inline assembly in function 'empty'
; SIDE-EFFECT-FREE: MMIXAL output variant 1 does not support inline assembly in function 'side_effect_free'
; OPERANDS: MMIXAL output variant 1 does not support inline assembly in function 'operands'
; ASM-GOTO: MMIXAL output variant 1 does not support inline assembly in function 'asm_goto'

;--- module.ll
target triple = "mmix-unknown-elf"

module asm "SWYM 0, 0, 0"

define void @Main() {
  br label %loop
loop:
  br label %loop
}

;--- empty.ll
target triple = "mmix-unknown-elf"

define void @Main() {
  br label %loop
loop:
  br label %loop
}

define void @empty() {
  call void asm "", ""()
  ret void
}

;--- side-effect-free.ll
target triple = "mmix-unknown-elf"

define void @Main() {
  br label %loop
loop:
  br label %loop
}

define void @side_effect_free() {
  call void asm "SWYM 0, 0, 0", ""()
  ret void
}

;--- operands.ll
target triple = "mmix-unknown-elf"

define void @Main() {
  br label %loop
loop:
  br label %loop
}

define i64 @operands(i64 %input) {
  %result = call i64 asm "OR $0, $1, 0", "=r,r"(i64 %input)
  ret i64 %result
}

;--- asm-goto.ll
target triple = "mmix-unknown-elf"

define void @Main() {
  br label %loop
loop:
  br label %loop
}

define void @asm_goto() {
entry:
  callbr void asm sideeffect "", "!i"()
      to label %fallthrough [label %target]
fallthrough:
  ret void
target:
  ret void
}
