; RUN: not llc -mtriple=mmix -O0 %s -o /dev/null 2>&1 | FileCheck %s

target triple = "mmix"

; A memory clobber cannot represent replacing the complete register and
; register-stack state of an ordinary function.
; CHECK: error: MMIX instruction 'SAVE' is only permitted in module-level inline assembly
; CHECK: error: MMIX instruction 'UNSAVE' is only permitted in module-level inline assembly

define void @function_context_state_assembly_is_rejected() {
  call void asm sideeffect "SAVE r250", "~{memory}"()
  call void asm sideeffect "UNSAVE r250", "~{memory}"()
  ret void
}
