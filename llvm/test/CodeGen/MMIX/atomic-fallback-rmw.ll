; RUN: llc -mtriple=mmix %s -o - | FileCheck %s

target triple = "mmix"

; An under-aligned RMW obtains and conditionally replaces the value through
; generic helpers. The retry loop preserves atomic fetch-add semantics.
define i32 @unaligned_atomic_rmw(ptr %p, i32 %value) {
; CHECK-LABEL: unaligned_atomic_rmw:
; CHECK: %geta(__atomic_load)
; CHECK: PUSHGO
; CHECK: %geta(__atomic_compare_exchange)
; CHECK: atomicrmw.start
; CHECK: PUSHGO
; CHECK: BZB
  %old = atomicrmw add ptr %p, i32 %value monotonic, align 2
  ret i32 %old
}
