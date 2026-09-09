; RUN: llc -mtriple=mmix -verify-machineinstrs -filetype=asm %s -o - | FileCheck %s

target triple = "mmix"

; CHECK-LABEL: add_registers:
; CHECK:       ADDU r231, r231, r232
; CHECK-NEXT:  POP 0, 0
define i64 @add_registers(i64 %lhs, i64 %rhs) nounwind {
  %result = add i64 %lhs, %rhs
  ret i64 %result
}

; CHECK-LABEL: add_immediate_max:
; CHECK:       ADDU r231, r231, 255
; CHECK-NEXT:  POP 0, 0
define i64 @add_immediate_max(i64 %value) nounwind {
  %result = add i64 %value, 255
  ret i64 %result
}

; CHECK-LABEL: add_immediate_out_of_range:
; CHECK:       SETL [[ADD256:r[0-9]+]], 256
; CHECK-NEXT:  ADDU r231, r231, [[ADD256]]
; CHECK-NEXT:  POP 0, 0
define i64 @add_immediate_out_of_range(i64 %value) nounwind {
  %result = add i64 %value, 256
  ret i64 %result
}

; CHECK-LABEL: add_negative_immediate:
; CHECK:       SUBU r231, r231, 255
; CHECK-NEXT:  POP 0, 0
define i64 @add_negative_immediate(i64 %value) nounwind {
  %result = add i64 %value, -255
  ret i64 %result
}

; CHECK-LABEL: add_negative_immediate_out_of_range:
; CHECK:       SETL [[NEG256:r[0-9]+]], 256
; CHECK-NEXT:  NEGU [[NEG256]], 0, [[NEG256]]
; CHECK-NEXT:  ADDU r231, r231, [[NEG256]]
; CHECK-NEXT:  POP 0, 0
define i64 @add_negative_immediate_out_of_range(i64 %value) nounwind {
  %result = add i64 %value, -256
  ret i64 %result
}

; CHECK-LABEL: subtract_registers:
; CHECK:       SUBU r231, r231, r232
; CHECK-NEXT:  POP 0, 0
define i64 @subtract_registers(i64 %lhs, i64 %rhs) nounwind {
  %result = sub i64 %lhs, %rhs
  ret i64 %result
}

; CHECK-LABEL: subtract_immediate_max:
; CHECK:       SUBU r231, r231, 255
; CHECK-NEXT:  POP 0, 0
define i64 @subtract_immediate_max(i64 %value) nounwind {
  %result = sub i64 %value, 255
  ret i64 %result
}

; CHECK-LABEL: subtract_immediate_out_of_range:
; CHECK:       SETL [[SUB256:r[0-9]+]], 256
; CHECK-NEXT:  NEGU [[SUB256]], 0, [[SUB256]]
; CHECK-NEXT:  ADDU r231, r231, [[SUB256]]
; CHECK-NEXT:  POP 0, 0
define i64 @subtract_immediate_out_of_range(i64 %value) nounwind {
  %result = sub i64 %value, 256
  ret i64 %result
}

; CHECK-LABEL: subtract_negative_immediate:
; CHECK:       ADDU r231, r231, 255
; CHECK-NEXT:  POP 0, 0
define i64 @subtract_negative_immediate(i64 %value) nounwind {
  %result = sub i64 %value, -255
  ret i64 %result
}

; CHECK-LABEL: negate:
; CHECK:       NEGU r231, 0, r231
; CHECK-NEXT:  POP 0, 0
define i64 @negate(i64 %value) nounwind {
  %result = sub i64 0, %value
  ret i64 %result
}

; Poison on signed overflow does not authorize MMIX's trapping NEG form.
; CHECK-LABEL: negate_nsw:
; CHECK:       NEGU r231, 0, r231
; CHECK-NEXT:  POP 0, 0
define i64 @negate_nsw(i64 %value) nounwind {
  %result = sub nsw i64 0, %value
  ret i64 %result
}

; LLVM overflow flags never select the trapping ADD or SUB instructions.
; CHECK-LABEL: add_nsw:
; CHECK:       ADDU r231, r231, r232
; CHECK-NEXT:  POP 0, 0
define i64 @add_nsw(i64 %lhs, i64 %rhs) nounwind {
  %result = add nsw i64 %lhs, %rhs
  ret i64 %result
}

; CHECK-LABEL: subtract_nuw:
; CHECK:       SUBU r231, r231, r232
; CHECK-NEXT:  POP 0, 0
define i64 @subtract_nuw(i64 %lhs, i64 %rhs) nounwind {
  %result = sub nuw i64 %lhs, %rhs
  ret i64 %result
}

; CHECK-LABEL: add_i8:
; CHECK:       ADDU r231, r231, r232
; CHECK-NEXT:  POP 0, 0
define i8 @add_i8(i8 %lhs, i8 %rhs) nounwind {
  %result = add i8 %lhs, %rhs
  ret i8 %result
}

; CHECK-LABEL: subtract_i32:
; CHECK:       SUBU r231, r231, r232
; CHECK-NEXT:  POP 0, 0
define i32 @subtract_i32(i32 %lhs, i32 %rhs) nounwind {
  %result = sub i32 %lhs, %rhs
  ret i32 %result
}
