; RUN: not llc -mtriple=mmix %s -o /dev/null 2>&1 | FileCheck %s
; CHECK: MMIX runtime unwind tables require a frameless leaf in function 'caller'
declare void @callee()
define void @caller() uwtable(sync) {
  call void @callee()
  ret void
}
