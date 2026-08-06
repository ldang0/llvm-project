; RUN: llc -mtriple=mmix -O0 < %s | FileCheck %s
; RUN: not llc -mtriple=mmix -mattr=-system -O0 < %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=NO-SYSTEM

target triple = "mmix"

define i64 @general_registers(i64 %x) {
; CHECK-LABEL: general_registers:
; CHECK:       #APP
; CHECK-NEXT:  ADDU [[GENERAL:r[0-9]+]], [[GENERAL]], 1
; CHECK:       #NO_APP
  %r = call i64 asm sideeffect "ADDU $0, $1, 1", "=r,r"(i64 %x)
  ret i64 %r
}

define i64 @read_write_register(i64 %x) {
; CHECK-LABEL: read_write_register:
; CHECK:       #APP
; CHECK-NEXT:  ADDU [[READWRITE:r[0-9]+]], [[READWRITE]], 1
; CHECK:       #NO_APP
  %r = call i64 asm sideeffect "ADDU $0, $0, 1", "=r,0"(i64 %x)
  ret i64 %r
}

define double @floating_registers(double %x) {
; CHECK-LABEL: floating_registers:
; CHECK:       #APP
; CHECK-NEXT:  OR [[FLOAT:r[0-9]+]], [[FLOAT]], 0
; CHECK:       #NO_APP
  %r = call double asm sideeffect "OR $0, $1, 0", "=r,r"(double %x)
  ret double %r
}

define i64 @fixed_registers(i64 %x) {
; CHECK-LABEL: fixed_registers:
; CHECK:       #APP
; CHECK-NEXT:  ADDU r0, r1, 1
; CHECK:       #NO_APP
  %r = call i64 asm sideeffect "ADDU $0, $1, 1", "={r0},{r1}"(i64 %x)
  ret i64 %r
}

define i64 @immediate_constraints(i64 %x) {
; CHECK-LABEL: immediate_constraints:
; CHECK:       ADDU {{r[0-9]+}}, {{r[0-9]+}}, 255
; CHECK:       SETL {{r[0-9]+}}, 65535
; CHECK:       SWYM 255, 0, 0
; CHECK:       SWYM 0, 0, 0
; CHECK:       SWYM 17, 0, 0
; CHECK:       SWYM 0, 0, 0
  %i = call i64 asm sideeffect "ADDU $0, $1, $2", "=r,r,I"(i64 %x, i64 255)
  %j = call i64 asm sideeffect "SETL $0, $1", "=r,J"(i64 65535)
  call void asm sideeffect "SWYM ${0:n}, 0, 0", "K"(i64 -255)
  call void asm sideeffect "SWYM $0, 0, 0", "M"(i64 0)
  call void asm sideeffect "SWYM $0, 0, 0", "O"(i64 17)
  call void asm sideeffect "SWYM $0, 0, 0", "G"(double 0.0)
  %r = add i64 %i, %j
  ret i64 %r
}

; MMIX has no condition-code register, so ~{cc} is a compatibility no-op.
; The other clobbers model hidden architectural and memory state.
define void @clobbers() {
; CHECK-LABEL: clobbers:
; CHECK:       #APP
; CHECK-NEXT:  SWYM 0, 0, 0
; CHECK:       #NO_APP
  call void asm sideeffect "SWYM 0, 0, 0", "~{rH},~{r255},~{memory},~{cc}"()
  ret void
}

define void @system_instruction() {
; CHECK-LABEL: system_instruction:
; CHECK:       #APP
; CHECK-NEXT:  SYNC 0
; CHECK:       #NO_APP
; NO-SYSTEM: error: instruction requires: system
  call void asm sideeffect "SYNC 0", "~{memory}"()
  ret void
}
