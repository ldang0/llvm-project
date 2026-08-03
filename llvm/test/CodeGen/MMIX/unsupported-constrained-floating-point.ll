; RUN: not --crash llc -mtriple=mmix -stop-after=mmix-isel \
; RUN:   -o /dev/null %s 2>&1 | FileCheck %s

; CHECK: MMIX constrained floating-point lowering is not implemented

define double @strict_add(double %lhs, double %rhs) strictfp {
  %result = call double @llvm.experimental.constrained.fadd.f64(
      double %lhs, double %rhs, metadata !"round.dynamic",
      metadata !"fpexcept.strict")
  ret double %result
}

declare double @llvm.experimental.constrained.fadd.f64(
    double, double, metadata, metadata)
