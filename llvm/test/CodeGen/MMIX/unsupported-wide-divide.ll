; RUN: not --crash llc -mtriple=mmix %s -o /dev/null 2>&1 | FileCheck %s

; CHECK: LLVM ERROR: unsupported library call operation

define i64 @wide_divide(i64 %lhs, i64 %rhs) {
  %lhs.wide = zext i64 %lhs to i128
  %rhs.wide = zext i64 %rhs to i128
  %quotient = udiv i128 %lhs.wide, %rhs.wide
  %result = trunc i128 %quotient to i64
  ret i64 %result
}
