; RUN: not llc -mtriple=mmix -filetype=asm %s -o %t.s 2>&1 | FileCheck %s
; RUN: test ! -s %t.s
; RUN: not llc -mtriple=mmix -filetype=obj %s -o %t.o 2>&1 | FileCheck %s
; RUN: test ! -s %t.o

; CHECK: MMIX supports only C and Fast calling conventions in function 'call_cold'

declare coldcc i64 @cold_callee(i64)

define i64 @call_cold(i64 %value) {
  %result = call coldcc i64 @cold_callee(i64 %value)
  ret i64 %result
}
