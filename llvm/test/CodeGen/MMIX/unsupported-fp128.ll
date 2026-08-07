; RUN: not --crash llc -mtriple=mmix %s -o /dev/null 2>&1 | FileCheck %s

; CHECK: LLVM ERROR: unsupported library call operation

define double @add_fp128(double %lhs, double %rhs) {
  %lhs.extended = fpext double %lhs to fp128
  %rhs.extended = fpext double %rhs to fp128
  %sum = fadd fp128 %lhs.extended, %rhs.extended
  %result = fptrunc fp128 %sum to double
  ret double %result
}
