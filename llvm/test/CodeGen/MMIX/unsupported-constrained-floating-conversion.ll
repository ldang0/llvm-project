; RUN: not --crash llc -mtriple=mmix -stop-after=mmix-isel \
; RUN:   -o /dev/null %s 2>&1 | FileCheck %s

; CHECK: MMIX constrained floating-point lowering is not implemented

define double @strict_signed_to_double(i64 %value) strictfp {
  %result = call double @llvm.experimental.constrained.sitofp.f64.i64(
      i64 %value, metadata !"round.tonearest", metadata !"fpexcept.strict")
  ret double %result
}

declare double @llvm.experimental.constrained.sitofp.f64.i64(
    i64, metadata, metadata)
