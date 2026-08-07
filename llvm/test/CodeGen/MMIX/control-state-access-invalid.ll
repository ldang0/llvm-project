; RUN: not llc -mtriple=mmix -O0 %s -o /dev/null 2>&1 | FileCheck %s

target triple = "mmix"

; CHECK: error: MMIX instruction 'TRAP' is only permitted in module-level inline assembly
; CHECK: error: MMIX instruction 'TRIP' is only permitted in module-level inline assembly
; CHECK: error: MMIX instruction 'RESUME' is only permitted in module-level inline assembly

define void @function_control_state_assembly_is_rejected() {
  call void asm sideeffect "TRAP 0, 0, 0", "~{memory}"()
  call void asm sideeffect "trip 1, 2, 3", "~{memory}"()
  call void asm sideeffect "continuation:\0A\09RESUME 0", "~{memory}"()
  ret void
}
