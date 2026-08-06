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
