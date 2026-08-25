; RUN: llc -mtriple=mmix -O0 -stop-after=finalize-isel \
; RUN:   -verify-machineinstrs %s -o - | FileCheck %s

%wide = type { i64, i64 }

declare i64 @scalar_target(i64)
declare void @caller_copy_target(ptr byval(%wide))
declare i64 @stack_target(i64, i64, i64, i64, i64, i64, i64, i64, i64,
                          i64, i64, i64, i64, i64, i64, i64, i64)

; CHECK-LABEL: name: dynamic_stack_fallback
; CHECK: hasTailCall: false
; CHECK: CALL_STATE @scalar_target
; CHECK-NOT: TAIL_STATE
define i64 @dynamic_stack_fallback(i64 %value) {
  %count = add i64 %value, 1
  %storage = alloca i8, i64 %count, align 8
  store volatile i8 0, ptr %storage, align 8
  %result = tail call i64 @scalar_target(i64 %value)
  ret i64 %result
}

; CHECK-LABEL: name: realigned_stack_fallback
; CHECK: hasTailCall: false
; CHECK: CALL_STATE @scalar_target
; CHECK-NOT: TAIL_STATE
define i64 @realigned_stack_fallback(i64 %value) {
  %storage = alloca i64, align 16
  store volatile i64 %value, ptr %storage, align 16
  %result = tail call i64 @scalar_target(i64 %value)
  ret i64 %result
}

; CHECK-LABEL: name: caller_copy_fallback
; CHECK: hasTailCall: false
; CHECK: CALL_STATE @caller_copy_target
; CHECK-NOT: TAIL_STATE
define void @caller_copy_fallback(ptr %value) {
  tail call void @caller_copy_target(ptr byval(%wide) %value)
  ret void
}

; CHECK-LABEL: name: outgoing_area_fallback
; CHECK: hasTailCall: false
; CHECK: CALL_STATE @stack_target
; CHECK-NOT: TAIL_STATE
define i64 @outgoing_area_fallback() {
  %result = tail call i64 @stack_target(
      i64 0, i64 1, i64 2, i64 3, i64 4, i64 5, i64 6, i64 7, i64 8,
      i64 9, i64 10, i64 11, i64 12, i64 13, i64 14, i64 15, i64 16)
  ret i64 %result
}

; CHECK-LABEL: name: unrestorable_frame_fallback
; CHECK: hasTailCall: false
; CHECK: CALL_STATE @scalar_target
; CHECK-NOT: TAIL_STATE
define i64 @unrestorable_frame_fallback(i64 %value) #0 {
  %result = tail call i64 @scalar_target(i64 %value)
  ret i64 %result
}

attributes #0 = { "probe-stack"="inline-asm" }
