; RUN: llc -mtriple=mmix -O0 -verify-machineinstrs < %s | FileCheck %s

declare i64 @llvm.mmix.trap(i64, i32 immarg, i32 immarg)

define i64 @semihosting_trap(i64 %argument) {
; CHECK-LABEL: semihosting_trap:
; CHECK:       OR r255, r231, 0
; CHECK-NEXT:  TRAP 0, 6, 1
; CHECK-NEXT:  OR r231, r255, 0
  %result = call i64 @llvm.mmix.trap(i64 %argument, i32 6, i32 1)
  ret i64 %result
}

define i64 @semihosting_trap_max_fields(i64 %argument) {
; CHECK-LABEL: semihosting_trap_max_fields:
; CHECK:       OR r255, r231, 0
; CHECK-NEXT:  TRAP 0, 255, 255
; CHECK-NEXT:  OR r231, r255, 0
  %result = call i64 @llvm.mmix.trap(i64 %argument, i32 255, i32 255)
  ret i64 %result
}
