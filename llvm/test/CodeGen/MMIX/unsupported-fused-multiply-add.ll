; RUN: not --crash llc -mtriple=mmix -stop-after=mmix-isel \
; RUN:   -o /dev/null %s 2>&1 | FileCheck %s

; CHECK: MMIX cannot lower fused f64 multiply-add without a runtime helper

define double @fused_multiply_add(double %lhs, double %rhs, double %addend) {
  %result = call double @llvm.fma.f64(double %lhs, double %rhs, double %addend)
  ret double %result
}

declare double @llvm.fma.f64(double, double, double)
