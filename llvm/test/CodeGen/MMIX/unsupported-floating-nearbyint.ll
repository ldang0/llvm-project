; RUN: not --crash llc -mtriple=mmix -stop-after=mmix-isel \
; RUN:   -o /dev/null %s 2>&1 | FileCheck %s

; CHECK: MMIX FINT cannot lower nearbyint because it may raise inexact

define double @nearbyint(double %value) {
  %result = call double @llvm.nearbyint.f64(double %value)
  ret double %result
}

declare double @llvm.nearbyint.f64(double)
