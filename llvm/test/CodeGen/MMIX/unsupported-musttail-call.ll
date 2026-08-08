; RUN: not llc -mtriple=mmix -stop-after=mmix-isel \
; RUN:   -o /dev/null %s 2>&1 | FileCheck %s

; CHECK: LLVM ERROR: MMIX does not support required tail calls in function 'required_tail'

declare i64 @callee(i64)

define i64 @required_tail(i64 %value) {
  %result = musttail call i64 @callee(i64 %value)
  ret i64 %result
}
