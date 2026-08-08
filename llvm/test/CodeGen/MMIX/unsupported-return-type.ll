; RUN: not llc -mtriple=mmix -filetype=asm %s -o /dev/null 2>&1 | FileCheck %s

; CHECK: LLVM ERROR: MMIX does not support aggregate or special formal arguments

define i128 @unsupported_wide_return() {
  ret i128 undef
}
