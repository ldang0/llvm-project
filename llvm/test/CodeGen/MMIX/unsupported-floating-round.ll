; RUN: not --crash llc -mtriple=mmix -stop-after=mmix-isel \
; RUN:   -o /dev/null %s 2>&1 | FileCheck %s

; CHECK: MMIX cannot lower round-to-nearest-ties-away without a runtime helper

define double @round_ties_away(double %value) {
  %result = call double @llvm.round.f64(double %value)
  ret double %result
}

declare double @llvm.round.f64(double)
