; RUN: llc -mtriple=mmix -O0 -verify-machineinstrs -filetype=asm %s -o - | FileCheck %s

target triple = "mmix"

; SAVE and UNSAVE replace register-stack, general-register, special-register,
; and memory state that the provisional function ABI cannot represent. A
; complete context routine must therefore own them as module-level assembly.
; r255 is always global, independent of the current rG threshold.
module asm ".text"
module asm "mmix_context_round_trip:"
module asm "SAVE r255"
module asm "UNSAVE r255"

; CHECK: mmix_context_round_trip:
; CHECK: SAVE r255
; CHECK: UNSAVE r255

declare void @callee()

; Ordinary frames, calls, and returns retain the provisional ABI and cannot
; acquire whole-context operations.
define i64 @ordinary_function_context() {
; CHECK-LABEL: ordinary_function_context:
; CHECK-NOT:   SAVE
; CHECK-NOT:   UNSAVE
; CHECK:       POP 0, 0
  %slot = alloca i64, align 8
  store volatile i64 42, ptr %slot, align 8
  call void @callee()
  %result = load volatile i64, ptr %slot, align 8
  ret i64 %result
}

; Context-state words in labels and comments are not instruction mnemonics.
define void @non_instruction_context_state_words() {
; CHECK-LABEL: non_instruction_context_state_words:
; CHECK:       #APP
; CHECK-NEXT:  SAVE:
; CHECK-NEXT:  # UNSAVE is a comment
; CHECK-NEXT:  SWYM 0, 0, 0
; CHECK:       #NO_APP
  call void asm sideeffect "SAVE:\0A\09# UNSAVE is a comment\0A\09SWYM 0, 0, 0", ""()
  ret void
}
