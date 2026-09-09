; RUN: llc -mtriple=mmix -verify-machineinstrs -filetype=asm %s -o - | FileCheck %s

target triple = "mmix"

; CHECK-LABEL: and_registers:
; CHECK:       AND r231, r231, r232
; CHECK-NEXT:  POP 0, 0
define i64 @and_registers(i64 %lhs, i64 %rhs) nounwind {
  %result = and i64 %lhs, %rhs
  ret i64 %result
}

; CHECK-LABEL: and_immediate:
; CHECK:       AND r231, r231, 255
; CHECK-NEXT:  POP 0, 0
define i64 @and_immediate(i64 %value) nounwind {
  %result = and i64 %value, 255
  ret i64 %result
}

; CHECK-LABEL: and_immediate_out_of_range:
; CHECK:       SETL [[AND256:r[0-9]+]], 256
; CHECK-NEXT:  AND r231, r231, [[AND256]]
; CHECK-NEXT:  POP 0, 0
define i64 @and_immediate_out_of_range(i64 %value) nounwind {
  %result = and i64 %value, 256
  ret i64 %result
}

; CHECK-LABEL: or_registers:
; CHECK:       OR r231, r231, r232
; CHECK-NEXT:  POP 0, 0
define i64 @or_registers(i64 %lhs, i64 %rhs) nounwind {
  %result = or i64 %lhs, %rhs
  ret i64 %result
}

; CHECK-LABEL: or_immediate:
; CHECK:       OR r231, r231, 255
; CHECK-NEXT:  POP 0, 0
define i64 @or_immediate(i64 %value) nounwind {
  %result = or i64 %value, 255
  ret i64 %result
}

; CHECK-LABEL: xor_registers:
; CHECK:       XOR r231, r231, r232
; CHECK-NEXT:  POP 0, 0
define i64 @xor_registers(i64 %lhs, i64 %rhs) nounwind {
  %result = xor i64 %lhs, %rhs
  ret i64 %result
}

; CHECK-LABEL: xor_immediate:
; CHECK:       XOR r231, r231, 255
; CHECK-NEXT:  POP 0, 0
define i64 @xor_immediate(i64 %value) nounwind {
  %result = xor i64 %value, 255
  ret i64 %result
}

; CHECK-LABEL: complement:
; CHECK:       NXOR r231, r231, 0
; CHECK-NEXT:  POP 0, 0
define i64 @complement(i64 %value) nounwind {
  %result = xor i64 %value, -1
  ret i64 %result
}

; CHECK-LABEL: and_complement:
; CHECK:       ANDN r231, r231, r232
; CHECK-NEXT:  POP 0, 0
define i64 @and_complement(i64 %lhs, i64 %rhs) nounwind {
  %not = xor i64 %rhs, -1
  %result = and i64 %lhs, %not
  ret i64 %result
}

; CHECK-LABEL: or_complement:
; CHECK:       ORN r231, r231, r232
; CHECK-NEXT:  POP 0, 0
define i64 @or_complement(i64 %lhs, i64 %rhs) nounwind {
  %not = xor i64 %rhs, -1
  %result = or i64 %lhs, %not
  ret i64 %result
}

; CHECK-LABEL: nor_registers:
; CHECK:       NOR r231, r231, r232
; CHECK-NEXT:  POP 0, 0
define i64 @nor_registers(i64 %lhs, i64 %rhs) nounwind {
  %or = or i64 %lhs, %rhs
  %result = xor i64 %or, -1
  ret i64 %result
}

; CHECK-LABEL: nand_registers:
; CHECK:       NAND r231, r231, r232
; CHECK-NEXT:  POP 0, 0
define i64 @nand_registers(i64 %lhs, i64 %rhs) nounwind {
  %and = and i64 %lhs, %rhs
  %result = xor i64 %and, -1
  ret i64 %result
}

; CHECK-LABEL: nxor_registers:
; CHECK:       NXOR r231, r231, r232
; CHECK-NEXT:  POP 0, 0
define i64 @nxor_registers(i64 %lhs, i64 %rhs) nounwind {
  %xor = xor i64 %lhs, %rhs
  %result = xor i64 %xor, -1
  ret i64 %result
}

; CHECK-LABEL: and_complemented_immediate:
; CHECK:       ANDN r231, r231, 255
; CHECK-NEXT:  POP 0, 0
define i64 @and_complemented_immediate(i64 %value) nounwind {
  %result = and i64 %value, -256
  ret i64 %result
}

; CHECK-LABEL: or_complemented_immediate:
; CHECK:       ORN r231, r231, 255
; CHECK-NEXT:  POP 0, 0
define i64 @or_complemented_immediate(i64 %value) nounwind {
  %result = or i64 %value, -256
  ret i64 %result
}

; CHECK-LABEL: nor_immediate:
; CHECK:       NOR r231, r231, 255
; CHECK-NEXT:  POP 0, 0
define i64 @nor_immediate(i64 %value) nounwind {
  %or = or i64 %value, 255
  %result = xor i64 %or, -1
  ret i64 %result
}

; CHECK-LABEL: nand_immediate:
; CHECK:       NAND r231, r231, 255
; CHECK-NEXT:  POP 0, 0
define i64 @nand_immediate(i64 %value) nounwind {
  %and = and i64 %value, 255
  %result = xor i64 %and, -1
  ret i64 %result
}

; CHECK-LABEL: nxor_immediate:
; CHECK:       NXOR r231, r231, 255
; CHECK-NEXT:  POP 0, 0
define i64 @nxor_immediate(i64 %value) nounwind {
  %xor = xor i64 %value, 255
  %result = xor i64 %xor, -1
  ret i64 %result
}

; A zero shift is eliminated before instruction selection.
; CHECK-LABEL: shift_left_zero:
; CHECK-NOT:   SL
; CHECK:       POP 0, 0
define i64 @shift_left_zero(i64 %value) {
  %result = shl i64 %value, 0
  ret i64 %result
}

; CHECK-LABEL: shift_left_immediate_boundary:
; CHECK:       SLU r231, r231, 63
; CHECK-NEXT:  POP 0, 0
define i64 @shift_left_immediate_boundary(i64 %value) nounwind {
  %result = shl i64 %value, 63
  ret i64 %result
}

; CHECK-LABEL: shift_left_register:
; CHECK:       SLU r231, r231, r232
; CHECK-NEXT:  POP 0, 0
define i64 @shift_left_register(i64 %value, i64 %amount) nounwind {
  %result = shl i64 %value, %amount
  ret i64 %result
}

; Even nsw does not permit MMIX's trapping SL instruction.
; CHECK-LABEL: shift_left_nsw:
; CHECK:       SLU r231, r231, 1
; CHECK-NEXT:  POP 0, 0
define i64 @shift_left_nsw(i64 %value) nounwind {
  %result = shl nsw i64 %value, 1
  ret i64 %result
}

; CHECK-LABEL: shift_right_arithmetic_immediate:
; CHECK:       SR r231, r231, 63
; CHECK-NEXT:  POP 0, 0
define i64 @shift_right_arithmetic_immediate(i64 %value) nounwind {
  %result = ashr i64 %value, 63
  ret i64 %result
}

; CHECK-LABEL: shift_right_arithmetic_register:
; CHECK:       SR r231, r231, r232
; CHECK-NEXT:  POP 0, 0
define i64 @shift_right_arithmetic_register(i64 %value, i64 %amount) nounwind {
  %result = ashr i64 %value, %amount
  ret i64 %result
}

; CHECK-LABEL: shift_right_logical_immediate:
; CHECK:       SRU r231, r231, 63
; CHECK-NEXT:  POP 0, 0
define i64 @shift_right_logical_immediate(i64 %value) nounwind {
  %result = lshr i64 %value, 63
  ret i64 %result
}

; CHECK-LABEL: shift_right_logical_register:
; CHECK:       SRU r231, r231, r232
; CHECK-NEXT:  POP 0, 0
define i64 @shift_right_logical_register(i64 %value, i64 %amount) nounwind {
  %result = lshr i64 %value, %amount
  ret i64 %result
}

; A source-level wrapping shift keeps the explicit mask that makes every count
; defined, then uses the register shift form.
; CHECK-LABEL: shift_left_masked_register:
; CHECK:       AND [[COUNT:r[0-9]+]], r232, 63
; CHECK-NEXT:  SLU r231, r231, [[COUNT]]
; CHECK-NEXT:  POP 0, 0
define i64 @shift_left_masked_register(i64 %value, i64 %amount) nounwind {
  %masked = and i64 %amount, 63
  %result = shl i64 %value, %masked
  ret i64 %result
}

; CHECK-LABEL: shift_left_i8:
; CHECK:       SLU r231, r231, 7
; CHECK-NEXT:  POP 0, 0
define i8 @shift_left_i8(i8 %value) nounwind {
  %result = shl i8 %value, 7
  ret i8 %result
}
