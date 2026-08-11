; RUN: not llc -mtriple=mmix -filetype=asm %s -o %t.s 2>&1 | FileCheck %s
; RUN: test ! -s %t.s
; RUN: not llc -mtriple=mmix -filetype=obj %s -o %t.o 2>&1 | FileCheck %s
; RUN: test ! -s %t.o

; CHECK: MMIX supports only the C calling convention in function 'call_fast'

declare fastcc i64 @fast_callee(i64)

define i64 @call_fast(i64 %value) {
  %result = call fastcc i64 @fast_callee(i64 %value)
  ret i64 %result
}
