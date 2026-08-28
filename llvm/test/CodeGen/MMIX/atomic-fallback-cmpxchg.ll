; RUN: llc -mtriple=mmix %s -o - | FileCheck %s

target triple = "mmix"

; An under-aligned compare-exchange uses the generic pointer-based GNU ABI
; helper instead of a sized helper or a native CSWAP.
define void @unaligned_cmpxchg(ptr %p) {
; CHECK-LABEL: unaligned_cmpxchg:
; CHECK: %geta(__atomic_compare_exchange)
; CHECK: PUSHGO
  %pair = cmpxchg ptr %p, i64 0, i64 1 monotonic monotonic, align 1
  ret void
}
