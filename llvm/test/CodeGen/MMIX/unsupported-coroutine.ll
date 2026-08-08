; RUN: not llc -mtriple=mmix -o /dev/null %s 2>&1 | FileCheck %s

; CHECK: LLVM ERROR: MMIX does not support coroutines in function 'coroutine'

define void @coroutine() presplitcoroutine {
  ret void
}
