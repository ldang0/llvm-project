; RUN: not --crash llc -mtriple=mmix -filetype=asm %s -o /dev/null 2>&1 | FileCheck %s

; CHECK: LLVM ERROR: MMIX supports only the C calling convention

define fastcc void @unsupported_fastcc() {
  ret void
}
