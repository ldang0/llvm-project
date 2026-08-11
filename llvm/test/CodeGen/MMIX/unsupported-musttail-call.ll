; RUN: not llc -mtriple=mmix -filetype=asm %s -o %t.s 2>&1 | FileCheck %s
; RUN: test ! -s %t.s
; RUN: not llc -mtriple=mmix -filetype=obj %s -o %t.o 2>&1 | FileCheck %s
; RUN: test ! -s %t.o

; CHECK: LLVM ERROR: MMIX does not support required tail calls in function 'required_tail'

declare i64 @callee(i64)

define i64 @required_tail(i64 %value) {
  %result = musttail call i64 @callee(i64 %value)
  ret i64 %result
}
