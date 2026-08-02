; RUN: not llc -mtriple=mmix -stop-after=mmix-isel -filetype=null \
; RUN:   -o /dev/null %s 2>&1 | FileCheck %s

; CHECK: error: MMIX instruction selection is not implemented

define void @pipeline_entry() {
  ret void
}
