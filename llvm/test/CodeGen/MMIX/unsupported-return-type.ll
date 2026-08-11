; RUN: not llc -mtriple=mmix -filetype=asm %s -o %t.s 2>&1 | FileCheck %s
; RUN: test ! -s %t.s
; RUN: not llc -mtriple=mmix -filetype=obj %s -o %t.o 2>&1 | FileCheck %s
; RUN: test ! -s %t.o

; CHECK: LLVM ERROR: MMIX does not support ABI type 'i128' for function results in function 'unsupported_wide_return'

define i128 @unsupported_wide_return() {
  ret i128 undef
}
