; RUN: llc -mtriple=mmix -verify-machineinstrs -filetype=asm %s -o - | FileCheck %s
; RUN: llc -mtriple=mmix -verify-machineinstrs -function-sections -filetype=asm %s -o - | FileCheck %s --check-prefix=SECTIONS

target triple = "mmix"

declare void @external_callee()

; CHECK-LABEL: earlier:
; CHECK:       POP 0, 0
define internal void @earlier() {
  ret void
}

; A call to an earlier definition uses the backward procedure form. Non-leaf
; functions preserve their incoming rJ in local register r30 and use r31 as
; the register-stack hole.
; CHECK-LABEL: call_backward:
; CHECK:       GET r30, rJ
; CHECK:       PUSHJB r31, earlier
; CHECK:       PUT rJ, r30
; CHECK-NEXT:  POP 0, 0
define void @call_backward() nounwind {
  call void @earlier()
  ret void
}

; Separate function sections do not have a known relative order, so even a
; local definition uses the conservative indirect procedure form.
; SECTIONS-LABEL: call_backward:
; SECTIONS:       GETA [[SECTION_TARGET:r[0-9]+]], %geta(earlier)
; SECTIONS:       PUSHGO r31, [[SECTION_TARGET]], 0

; CHECK-LABEL: call_forward:
; CHECK:       GET r30, rJ
; CHECK:       PUSHJ r31, later
; CHECK:       PUT rJ, r30
; CHECK-NEXT:  POP 0, 0
define void @call_forward() nounwind {
  call void @later()
  ret void
}

define internal void @later() {
  ret void
}

; Canonical text uses relocatable address materialization plus PUSHGO for
; unresolved direct symbols.
; CHECK-LABEL: call_external:
; CHECK:       GET r30, rJ
; CHECK:       GETA [[TARGET:r[0-9]+]], %geta(external_callee)
; CHECK:       PUSHGO r31, [[TARGET]], 0
; CHECK:       PUT rJ, r30
; CHECK-NEXT:  POP 0, 0
define void @call_external() nounwind {
  call void @external_callee()
  ret void
}

; CHECK-LABEL: call_indirect:
; CHECK:       GET r30, rJ
; CHECK:       PUSHGO r31, r231, 0
; CHECK:       PUT rJ, r30
; CHECK-NEXT:  POP 0, 0
define void @call_indirect(ptr %callee) nounwind {
  call void %callee()
  ret void
}

; Recursive calls are backward from their own instruction address.
; CHECK-LABEL: recursive:
; CHECK:       PUSHJB r31, recursive
define void @recursive() {
  call void @recursive()
  ret void
}
