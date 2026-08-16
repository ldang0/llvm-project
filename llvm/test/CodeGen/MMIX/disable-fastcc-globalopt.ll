; RUN: opt -mtriple=mmix -passes=globalopt -S %s | FileCheck %s

define i64 @caller(i64 %value) {
; CHECK-LABEL: define i64 @caller(
; CHECK:         %result = call i64 @callee(i64 %value)
  %result = call i64 @callee(i64 %value)
  ret i64 %result
}

define internal i64 @callee(i64 %value) noinline {
; CHECK-LABEL: define internal i64 @callee(
  %result = add i64 %value, 1
  ret i64 %result
}
