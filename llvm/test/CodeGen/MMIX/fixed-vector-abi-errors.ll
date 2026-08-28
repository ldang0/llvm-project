; RUN: split-file %s %t
; RUN: not llc -mtriple=mmix %t/wide.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=WIDE
; RUN: not llc -mtriple=mmix %t/variadic.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=VARIADIC
; RUN: not llc -mtriple=mmix %t/variadic-call.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=VARIADIC-CALL
; RUN: not llc -mtriple=mmix %t/musttail.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=MUSTTAIL

;--- wide.ll
target triple = "mmix"
define <4 x i32> @wide(<4 x i32> %value) {
  ret <4 x i32> %value
}
; WIDE: LLVM ERROR: MMIX does not support ABI type '<4 x i32>' for formal arguments in function 'wide'

;--- variadic.ll
target triple = "mmix"
define void @variadic(<2 x i32> %value, ...) {
  ret void
}
; VARIADIC: LLVM ERROR: MMIX does not support variadic or split fixed-vector formal arguments in function 'variadic'

;--- variadic-call.ll
target triple = "mmix"
declare void @sink(i64, ...)
define void @variadic_call(<2 x i32> %value) {
  call void (i64, ...) @sink(i64 0, <2 x i32> %value)
  ret void
}
; VARIADIC-CALL: LLVM ERROR: MMIX does not support variadic or split fixed-vector call arguments in function 'variadic_call'

;--- musttail.ll
target triple = "mmix"
declare <2 x i32> @callee(<2 x i32>)
define <2 x i32> @musttail_vector(<2 x i32> %value) {
  %result = musttail call <2 x i32> @callee(<2 x i32> %value)
  ret <2 x i32> %result
}
; MUSTTAIL: LLVM ERROR: MMIX required tail call is ineligible in function 'musttail_vector': callee argument locations are incompatible
