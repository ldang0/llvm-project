; RUN: not llc -mtriple=mmix -filetype=asm %s -o /dev/null 2>&1 | FileCheck %s

; CHECK: LLVM ERROR: MMIX does not support variadic functions

define void @unsupported_varargs(i64 %fixed, ...) {
  ret void
}
