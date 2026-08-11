; RUN: not llc -mtriple=mmix -O0 %s -o /dev/null 2>&1 | FileCheck %s

target triple = "mmix"

; CHECK: error: MMIX instruction 'PUSHJ' is only permitted in module-level inline assembly
; CHECK: error: MMIX instruction 'PUSHGO' is only permitted in module-level inline assembly
; CHECK: error: MMIX instruction 'POP' is only permitted in module-level inline assembly

define void @function_procedure_state_assembly_is_rejected() {
  call void asm sideeffect "PUSHJ r31, target", ""()
  call void asm sideeffect "pushgo r31, r0, 0", ""()
  call void asm sideeffect "continuation:\0A\09POP 0, 0", ""()
  ret void
}
