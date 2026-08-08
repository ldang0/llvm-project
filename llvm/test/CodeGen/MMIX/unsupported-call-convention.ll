; RUN: not llc -mtriple=mmix -stop-after=mmix-isel \
; RUN:   -o /dev/null %s 2>&1 | FileCheck %s

; CHECK: MMIX supports only the C calling convention

declare fastcc i64 @fast_callee(i64)

define i64 @call_fast(i64 %value) {
  %result = call fastcc i64 @fast_callee(i64 %value)
  ret i64 %result
}
