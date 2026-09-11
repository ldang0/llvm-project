; RUN: split-file %s %t
; RUN: not llc -mtriple=mmix -O0 -verify-machineinstrs %t/TRAP.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=TRAP
; RUN: not llc -mtriple=mmix -O0 -verify-machineinstrs %t/TRIP.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=TRIP
; RUN: not llc -mtriple=mmix -O0 -verify-machineinstrs %t/RESUME.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=RESUME
; RUN: not llc -mtriple=mmix -O0 -verify-machineinstrs %t/GO.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=GO

; TRAP: LLVM ERROR: MMIX instruction 'TRAP' is only permitted in module-level inline assembly
; TRIP: LLVM ERROR: MMIX instruction 'TRIP' is only permitted in module-level inline assembly
; RESUME: LLVM ERROR: MMIX instruction 'RESUME' is only permitted in module-level inline assembly
; GO: LLVM ERROR: MMIX instruction 'GO' is only permitted in module-level inline assembly

;--- TRAP.ll
define void @rejected() {
  call void asm sideeffect "TRAP 0, 0, 0", "~{memory}"()
  ret void
}

;--- TRIP.ll
define void @rejected() {
  call void asm sideeffect "trip 1, 2, 3", "~{memory}"()
  ret void
}

;--- RESUME.ll
define void @rejected() {
  call void asm sideeffect "continuation:\0A\09RESUME 0", "~{memory}"()
  ret void
}

;--- GO.ll
define void @rejected() {
  call void asm sideeffect "GO r0, r1, 0", "~{r0},~{memory}"()
  ret void
}
