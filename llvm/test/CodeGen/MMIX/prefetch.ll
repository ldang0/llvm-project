; RUN: llc -mtriple=mmix -verify-machineinstrs %s -o - | FileCheck %s

target triple = "mmix"

; Prefetch is an optional hint. Until a cache-policy contract is defined,
; dropping it is preferable to assigning PRELD or another system instruction
; semantics that generic LLVM IR does not guarantee.
define void @prefetch_read(ptr %address) {
; CHECK-LABEL: prefetch_read:
; CHECK-NOT:   PRELD
; CHECK-NOT:   PREGO
; CHECK-NOT:   PREST
; CHECK:       POP 0, 0
  call void @llvm.prefetch.p0(ptr %address, i32 0, i32 3, i32 1)
  ret void
}

declare void @llvm.prefetch.p0(ptr, i32, i32, i32)
