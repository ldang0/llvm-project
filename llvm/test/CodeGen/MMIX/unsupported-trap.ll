; RUN: not --crash llc -mtriple=mmix %s -o /dev/null 2>&1 | FileCheck %s

; CHECK: LLVM ERROR: MMIX does not define an LLVM trap convention for this environment

define void @trap() {
  call void @llvm.trap()
  unreachable
}

declare void @llvm.trap()
