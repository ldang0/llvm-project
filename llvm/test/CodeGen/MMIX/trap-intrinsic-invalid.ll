; RUN: not llc -mtriple=mmix -O0 -verify-machineinstrs < %s -o /dev/null 2>&1 | FileCheck %s

declare i64 @llvm.mmix.trap(i64, i32 immarg, i32 immarg)

define i64 @out_of_range_service(i64 %argument) {
; CHECK: error: llvm.mmix.trap: service and handle must fit in one byte
  %result = call i64 @llvm.mmix.trap(i64 %argument, i32 256, i32 0)
  ret i64 %result
}
