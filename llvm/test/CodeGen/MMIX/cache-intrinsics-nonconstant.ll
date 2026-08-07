; RUN: not llc -mtriple=mmix %s -o /dev/null 2>&1 | FileCheck %s

; CHECK: immarg operand has non-immediate parameter

declare void @llvm.mmix.preld(ptr, i32 immarg)
declare void @llvm.mmix.sync(i32 immarg)

define void @span_nonconstant(ptr %address, i32 %span) {
  call void @llvm.mmix.preld(ptr %address, i32 %span)
  ret void
}

define void @mode_nonconstant(i32 %mode) {
  call void @llvm.mmix.sync(i32 %mode)
  ret void
}
