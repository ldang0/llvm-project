; RUN: not llc -mtriple=mmix -filetype=asm %s -o %t.s 2>&1 | FileCheck %s
; RUN: test ! -s %t.s
; RUN: not llc -mtriple=mmix -filetype=obj %s -o %t.o 2>&1 | FileCheck %s
; RUN: test ! -s %t.o

; CHECK: LLVM ERROR: MMIX does not support ABI type '<vscale x 2 x i64>' for formal arguments in function 'scalable_vector_argument'

define i64 @scalable_vector_argument(<vscale x 2 x i64> %vector) {
  %result = extractelement <vscale x 2 x i64> %vector, i64 0
  ret i64 %result
}
