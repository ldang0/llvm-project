; RUN: not llc -mtriple=mmix %s -o /dev/null 2>&1 | FileCheck %s

; CHECK: LLVM ERROR: MMIX does not support aggregate or special formal arguments

define i64 @scalable_vector_argument(<vscale x 2 x i64> %vector) {
  %result = extractelement <vscale x 2 x i64> %vector, i64 0
  ret i64 %result
}
