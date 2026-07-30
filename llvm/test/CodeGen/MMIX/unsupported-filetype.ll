; RUN: not llc -mtriple=mmix -filetype=null -o /dev/null %s 2>&1 | FileCheck %s

; CHECK: llc: error: target does not support generation of this file type

define void @smoke() {
  ret void
}
