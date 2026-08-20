; RUN: llc -mtriple=mmix %s -o - | FileCheck %s

target triple = "mmix"

; An under-aligned RMW obtains and conditionally replaces the value through
; generic helpers. The retry loop preserves atomic fetch-add semantics.
define i32 @unaligned_atomic_rmw(ptr %p, i32 %value) {
; CHECK-LABEL: unaligned_atomic_rmw:
; CHECK: (__atomic_load>>48)
; CHECK: PUSHGO
; CHECK: (__atomic_compare_exchange>>48)
; CHECK: atomicrmw.start
; CHECK: PUSHGO
; CHECK: BZB
  %old = atomicrmw add ptr %p, i32 %value monotonic, align 2
  ret i32 %old
}
