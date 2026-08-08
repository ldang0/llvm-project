; RUN: not llc -mtriple=mmix -stop-after=mmix-isel \
; RUN:   -o /dev/null %s 2>&1 | FileCheck %s

; CHECK: MMIX does not support aggregate or special call arguments

%aggregate = type { i64, i64 }

declare void @aggregate_callee(ptr byval(%aggregate))

define void @call_aggregate(ptr %value) {
  call void @aggregate_callee(ptr byval(%aggregate) %value)
  ret void
}
