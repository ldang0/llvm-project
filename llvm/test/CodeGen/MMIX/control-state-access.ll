; RUN: llc -mtriple=mmix -O0 -verify-machineinstrs -filetype=asm %s -o - | FileCheck %s

target triple = "mmix"

; Trap, trip, and resume sequences own control flow and machine state that the
; provisional function ABI cannot represent. Complete entry and handler
; sequences therefore remain available only as module-level assembly.
module asm ".text"
module asm "mmix_trip_entry:"
module asm "TRIP 1, 2, 3"
module asm "RESUME 0"
module asm "mmix_trap_entry:"
module asm "TRAP 0, 0, 0"
module asm "RESUME 1"

; CHECK:      mmix_trip_entry:
; CHECK:      TRIP 1, 2, 3
; CHECK:      RESUME 0
; CHECK:      mmix_trap_entry:
; CHECK:      TRAP 0, 0, 0
; CHECK:      RESUME 1

declare void @callee()

; Ordinary branches, calls, and returns retain their generic CodeGen
; conventions and cannot acquire raw control-state instructions.
define void @ordinary_control_flow(i1 %condition) {
; CHECK-LABEL: ordinary_control_flow:
; CHECK-NOT:   TRAP
; CHECK-NOT:   TRIP
; CHECK-NOT:   RESUME
; CHECK:       POP 0, 0
entry:
  br i1 %condition, label %call, label %return

call:
  call void @callee()
  br label %return

return:
  ret void
}

; Control-state words in labels and comments are not instruction mnemonics.
define void @non_instruction_control_state_words() {
; CHECK-LABEL: non_instruction_control_state_words:
; CHECK:       #APP
; CHECK-NEXT:  TRAP:
; CHECK-NEXT:  # TRIP and RESUME are comments
; CHECK-NEXT:  SWYM 0, 0, 0
; CHECK:       #NO_APP
  call void asm sideeffect "TRAP:\0A\09# TRIP and RESUME are comments\0A\09SWYM 0, 0, 0", ""()
  ret void
}
