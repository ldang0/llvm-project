; RUN: split-file %s %t
; RUN: not llc -mtriple=mmix -O0 -verify-machineinstrs %t/SAVE.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=SAVE
; RUN: not llc -mtriple=mmix -O0 -verify-machineinstrs %t/UNSAVE.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=UNSAVE

; SAVE: LLVM ERROR: MMIX instruction 'SAVE' is only permitted in module-level inline assembly
; UNSAVE: LLVM ERROR: MMIX instruction 'UNSAVE' is only permitted in module-level inline assembly

; A memory clobber cannot represent replacing the complete register and
; register-stack state of an ordinary function.

;--- SAVE.ll
define void @rejected() {
  call void asm sideeffect "SAVE r250", "~{memory}"()
  ret void
}

;--- UNSAVE.ll
define void @rejected() {
  call void asm sideeffect "UNSAVE r250", "~{memory}"()
  ret void
}
