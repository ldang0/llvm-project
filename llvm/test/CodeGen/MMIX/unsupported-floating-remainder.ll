; RUN: not --crash llc -mtriple=mmix -stop-after=mmix-isel \
; RUN:   -o /dev/null %s 2>&1 | FileCheck %s

; CHECK: MMIX cannot directly lower LLVM frem:
; CHECK-SAME: MMIX FREM implements IEEE remainder
; CHECK-SAME: instead of truncating-quotient fmod semantics

define double @unsupported_floating_remainder(double %lhs, double %rhs) {
  %result = frem double %lhs, %rhs
  ret double %result
}
