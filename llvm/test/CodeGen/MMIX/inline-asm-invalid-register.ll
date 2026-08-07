; RUN: not llc -mtriple=mmix -O0 < %s -o /dev/null 2>&1 | FileCheck %s

target triple = "mmix"

define i64 @special_register_value(i64 %x) {
; CHECK: error: could not allocate input reg for constraint '{rH}'
  %r = call i64 asm sideeffect "OR $0, $1, 0", "=r,{rH}"(i64 %x)
  ret i64 %r
}

define i64 @reserved_register_value(i64 %x) {
; CHECK: error: could not allocate input reg for constraint '{r255}'
  %r = call i64 asm sideeffect "OR $0, $1, 0", "=r,{r255}"(i64 %x)
  ret i64 %r
}

define i64 @return_state_register(i64 %x) {
; CHECK: error: could not allocate output register for constraint '{r30}'
  %r = call i64 asm sideeffect "OR $0, $1, 0", "={r30},r"(i64 %x)
  ret i64 %r
}

define void @unsafe_special_clobber() {
; CHECK: error: MMIX inline assembly may not clobber register 'rA' in an ordinary function
  call void asm sideeffect "SWYM 0, 0, 0", "~{rA}"()
  ret void
}

define void @stack_pointer_clobber() {
; CHECK: error: MMIX inline assembly may not clobber register 'r254' in an ordinary function
  call void asm sideeffect "SWYM 0, 0, 0", "~{r254}"()
  ret void
}

define void @unsafe_special_register_writes() {
; CHECK: error: MMIX instruction 'PUT rJ' is only permitted in module-level inline assembly
; CHECK: error: MMIX instruction 'PUT rG' is only permitted in module-level inline assembly
; CHECK: error: MMIX instruction 'PUT rL' is only permitted in module-level inline assembly
; CHECK: error: MMIX instruction 'PUT rA' is only permitted in module-level inline assembly
; CHECK: error: MMIX instruction 'PUT rN' is only permitted in module-level inline assembly
; CHECK: error: MMIX instruction 'PUT rO' is only permitted in module-level inline assembly
; CHECK: error: MMIX instruction 'PUT rS' is only permitted in module-level inline assembly
  call void asm sideeffect "PUT rJ, r0", ""()
  call void asm sideeffect "put rG, r0", ""()
  call void asm sideeffect "label:\0A\09PUT rL, r0", ""()
  call void asm sideeffect "PUT rA, r0", ""()
  call void asm sideeffect "PUT rN, r0", ""()
  call void asm sideeffect "PUT rO, r0", ""()
  call void asm sideeffect "PUT rS, r0", ""()
  ret void
}
