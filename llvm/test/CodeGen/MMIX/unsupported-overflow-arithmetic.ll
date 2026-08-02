; RUN: llc -mtriple=mmix -verify-machineinstrs -filetype=asm %s -o - | FileCheck %s

; Overflow-reporting arithmetic is expanded without selecting MMIX's trapping
; signed instructions.
; CHECK-LABEL: unsigned_add_overflow:
; CHECK-NOT:   ADD {{[^U]}}
; CHECK:       ADDU
; CHECK:       CMPU
; CHECK:       ZSN
; CHECK:       POP 0, 0

declare { i64, i1 } @llvm.uadd.with.overflow.i64(i64, i64)

define i1 @unsigned_add_overflow(i64 %lhs, i64 %rhs) {
  %result = call { i64, i1 } @llvm.uadd.with.overflow.i64(i64 %lhs, i64 %rhs)
  %overflow = extractvalue { i64, i1 } %result, 1
  ret i1 %overflow
}
