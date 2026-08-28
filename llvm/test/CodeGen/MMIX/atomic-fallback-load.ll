; RUN: llc -mtriple=mmix %s -o - | FileCheck %s

target triple = "mmix"

; A native-width load whose declared alignment is insufficient uses the
; generic helper, which applies the runtime alignment and locking policy.
define i32 @unaligned_atomic_load(ptr %p) {
; CHECK-LABEL: unaligned_atomic_load:
; CHECK: %geta(__atomic_load)
; CHECK: PUSHGO
  %value = load atomic i32, ptr %p monotonic, align 2
  ret i32 %value
}
