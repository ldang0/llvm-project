; RUN: not --crash llc -mtriple=mmix -stop-after=mmix-isel \
; RUN:   -o /dev/null %s 2>&1 | FileCheck %s

; CHECK: MMIX does not support variadic calls

declare void @variadic(i64, ...)

define void @call_variadic(i64 %value) {
  call void (i64, ...) @variadic(i64 %value, i64 1)
  ret void
}
