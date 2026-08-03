; RUN: not --crash llc -mtriple=mmix -stop-after=mmix-isel \
; RUN:   -o /dev/null %s 2>&1 | FileCheck %s

; CHECK: MMIX SelectionDAG operation is not implemented by this lowering stage: fadd

define double @unsupported_floating_point(double %lhs, double %rhs) {
  %result = fadd double %lhs, %rhs
  ret double %result
}
