; RUN: not llc -mtriple=mmix %s -o /dev/null 2>&1 | FileCheck %s

; CHECK: immarg operand has non-immediate parameter
; CHECK: call void @llvm.mmix.preld
; CHECK: immarg operand has non-immediate parameter
; CHECK: call void @llvm.mmix.prego
; CHECK: immarg operand has non-immediate parameter
; CHECK: call void @llvm.mmix.prest
; CHECK: immarg operand has non-immediate parameter
; CHECK: call void @llvm.mmix.syncd
; CHECK: immarg operand has non-immediate parameter
; CHECK: call void @llvm.mmix.syncid
; CHECK: immarg operand has non-immediate parameter
; CHECK: call void @llvm.mmix.sync

declare void @llvm.mmix.preld(ptr, i32 immarg)
declare void @llvm.mmix.prego(ptr, i32 immarg)
declare void @llvm.mmix.prest(ptr, i32 immarg)
declare void @llvm.mmix.syncd(ptr, i32 immarg)
declare void @llvm.mmix.syncid(ptr, i32 immarg)
declare void @llvm.mmix.sync(i32 immarg)

define void @preld_span_nonconstant(ptr %address, i32 %span) {
  call void @llvm.mmix.preld(ptr %address, i32 %span)
  ret void
}

define void @prego_span_nonconstant(ptr %address, i32 %span) {
  call void @llvm.mmix.prego(ptr %address, i32 %span)
  ret void
}

define void @prest_span_nonconstant(ptr %address, i32 %span) {
  call void @llvm.mmix.prest(ptr %address, i32 %span)
  ret void
}

define void @syncd_span_nonconstant(ptr %address, i32 %span) {
  call void @llvm.mmix.syncd(ptr %address, i32 %span)
  ret void
}

define void @syncid_span_nonconstant(ptr %address, i32 %span) {
  call void @llvm.mmix.syncid(ptr %address, i32 %span)
  ret void
}

define void @mode_nonconstant(i32 %mode) {
  call void @llvm.mmix.sync(i32 %mode)
  ret void
}
