; RUN: not llvm-as %s -o /dev/null 2>&1 | FileCheck %s

target triple = "mmix-unknown-unknown"

declare fastcc i64 @fast_callee(i64)

define i64 @mismatched_calling_convention(i64 %value) {
; CHECK: cannot guarantee tail call due to mismatched calling conv
  %result = musttail call fastcc i64 @fast_callee(i64 %value)
  ret i64 %result
}

declare i64 @extra_parameter_callee(i64, i64)

define i64 @mismatched_parameter_count(i64 %value) {
; CHECK: cannot guarantee tail call due to mismatched parameter counts
  %result = musttail call i64 @extra_parameter_callee(i64 %value, i64 0)
  ret i64 %result
}

declare i32 @narrow_result_callee(i64)

define i64 @mismatched_result(i64 %value) {
; CHECK: cannot guarantee tail call due to mismatched return types
  %result = musttail call i32 @narrow_result_callee(i64 %value)
  %extended = zext i32 %result to i64
  ret i64 %extended
}

declare i64 @not_tail_position_callee(i64)

define i64 @not_in_tail_position(i64 %value) {
; CHECK: musttail call must precede a ret
  %result = musttail call i64 @not_tail_position_callee(i64 %value)
  %adjusted = add i64 %result, 1
  ret i64 %adjusted
}
