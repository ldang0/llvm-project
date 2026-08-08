; RUN: not llc -mtriple=mmix -filetype=asm %s -o /dev/null 2>&1 | FileCheck %s

; CHECK: LLVM ERROR: MMIX does not support aggregate or special formal arguments

define void @unsupported_wide_argument(i128 %value) {
  ret void
}
