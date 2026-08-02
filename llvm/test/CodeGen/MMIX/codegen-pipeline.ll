; RUN: llc -mtriple=mmix -stop-after=mmix-isel -o - %s | FileCheck %s
; RUN: llc -mtriple=mmix -filetype=asm -o - %s | FileCheck %s \
; RUN:   --check-prefix=ASM

; CHECK-LABEL: name: pipeline_entry
; CHECK: failedISel: false
; CHECK: body:
; CHECK-NEXT: bb.0.entry:

; ASM-LABEL: pipeline_entry:
; ASM-NOT: MMIX instruction selection is not implemented
; ASM: .Lfunc_end0:

define void @pipeline_entry() {
entry:
  unreachable
}
