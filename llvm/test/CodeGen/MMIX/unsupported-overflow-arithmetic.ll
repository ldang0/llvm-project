; RUN: not --crash llc -mtriple=mmix -filetype=asm %s -o /dev/null 2>&1 | FileCheck %s

; Overflow-reporting arithmetic is expanded without selecting MMIX's trapping
; signed instructions. Comparison lowering will complete this expansion.
; CHECK: MMIX SelectionDAG operation is not implemented by this lowering stage: setcc

declare { i64, i1 } @llvm.uadd.with.overflow.i64(i64, i64)

define i1 @unsigned_add_overflow(i64 %lhs, i64 %rhs) {
  %result = call { i64, i1 } @llvm.uadd.with.overflow.i64(i64 %lhs, i64 %rhs)
  %overflow = extractvalue { i64, i1 } %result, 1
  ret i1 %overflow
}
