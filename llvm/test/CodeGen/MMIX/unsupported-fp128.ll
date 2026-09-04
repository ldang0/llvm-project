; RUN: rm -f %t.o
; RUN: not llc -mtriple=mmix %s -o %t.o 2>&1 | FileCheck %s
; RUN: test ! -e %t.o

; CHECK-COUNT-2: error: no libcall available for fp_extend
; CHECK-NEXT: error: no libcall available for fadd
; CHECK-NEXT: error: no libcall available for fp_round

define double @add_fp128(double %lhs, double %rhs) {
  %lhs.extended = fpext double %lhs to fp128
  %rhs.extended = fpext double %rhs to fp128
  %sum = fadd fp128 %lhs.extended, %rhs.extended
  %result = fptrunc fp128 %sum to double
  ret double %result
}
