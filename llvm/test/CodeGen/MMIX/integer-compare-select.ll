; RUN: llc -mtriple=mmix -verify-machineinstrs -filetype=asm %s -o - | FileCheck %s

target triple = "mmix"

; CHECK-LABEL: compare_signed:
; CHECK:       CMP [[CMP:r[0-9]+]], r231, r232
; CHECK-NEXT:  ZSN r231, [[CMP]], 1
; CHECK-NEXT:  POP 0, 0
define i1 @compare_signed(i64 %lhs, i64 %rhs) nounwind {
  %result = icmp slt i64 %lhs, %rhs
  ret i1 %result
}

; CHECK-LABEL: compare_signed_less_equal:
; CHECK:       CMP [[CMP:r[0-9]+]], r231, r232
; CHECK-NEXT:  ZSNP r231, [[CMP]], 1
; CHECK-NEXT:  POP 0, 0
define i1 @compare_signed_less_equal(i64 %lhs, i64 %rhs) nounwind {
  %result = icmp sle i64 %lhs, %rhs
  ret i1 %result
}

; CHECK-LABEL: compare_signed_greater:
; CHECK:       CMP [[CMP:r[0-9]+]], r231, r232
; CHECK-NEXT:  ZSP r231, [[CMP]], 1
; CHECK-NEXT:  POP 0, 0
define i1 @compare_signed_greater(i64 %lhs, i64 %rhs) nounwind {
  %result = icmp sgt i64 %lhs, %rhs
  ret i1 %result
}

; CHECK-LABEL: compare_unsigned:
; CHECK:       CMPU [[CMP:r[0-9]+]], r231, r232
; CHECK-NEXT:  ZSNP r231, [[CMP]], 1
; CHECK-NEXT:  POP 0, 0
define i1 @compare_unsigned(i64 %lhs, i64 %rhs) nounwind {
  %result = icmp ule i64 %lhs, %rhs
  ret i1 %result
}

; CHECK-LABEL: compare_equal:
; CHECK:       CMPU [[CMP:r[0-9]+]], r231, r232
; CHECK-NEXT:  ZSZ r231, [[CMP]], 1
; CHECK-NEXT:  POP 0, 0
define i1 @compare_equal(i64 %lhs, i64 %rhs) nounwind {
  %result = icmp eq i64 %lhs, %rhs
  ret i1 %result
}

; CHECK-LABEL: compare_immediate:
; CHECK:       CMPU [[CMP:r[0-9]+]], r231, 255
; CHECK-NEXT:  ZSP r231, [[CMP]], 1
; CHECK-NEXT:  POP 0, 0
define i1 @compare_immediate(i64 %value) nounwind {
  %result = icmp ugt i64 %value, 255
  ret i1 %result
}

; CHECK-LABEL: compare_signed_immediate:
; CHECK:       CMP [[CMP:r[0-9]+]], r231, 255
; CHECK-NEXT:  ZSN r231, [[CMP]], 1
; CHECK-NEXT:  POP 0, 0
define i1 @compare_signed_immediate(i64 %value) nounwind {
  %result = icmp slt i64 %value, 255
  ret i1 %result
}

; CHECK-LABEL: compare_signed_i8:
; CHECK:       SLU
; CHECK-NEXT:  SR
; CHECK:       CMP
; CHECK:       ZSNN r231,
; CHECK:       POP 0, 0
define i1 @compare_signed_i8(i8 %lhs, i8 %rhs) {
  %result = icmp sge i8 %lhs, %rhs
  ret i1 %result
}

; CHECK-LABEL: compare_unsigned_i32:
; CHECK:       AND
; CHECK:       AND
; CHECK:       CMPU
; CHECK:       ZSNZ r231,
; CHECK:       POP 0, 0
define i1 @compare_unsigned_i32(i32 %lhs, i32 %rhs) {
  %result = icmp ne i32 %lhs, %rhs
  ret i1 %result
}

; Pointer comparisons use the same unsigned octabyte relation as i64 values.
; CHECK-LABEL: compare_pointer:
; CHECK:       CMPU
; CHECK-NEXT:  ZSN r231,
; CHECK:       POP 0, 0
define i1 @compare_pointer(ptr %lhs, ptr %rhs) {
  %result = icmp ult ptr %lhs, %rhs
  ret i1 %result
}

; CHECK-LABEL: select_registers:
; CHECK:       CMP
; CHECK:       ZSNZ [[COND:r[0-9]+]],
; CHECK:       CSNZ r234, [[COND]], r233
; CHECK-NEXT:  OR r231, r234, 0
; CHECK-NEXT:  POP 0, 0
define i64 @select_registers(i64 %lhs, i64 %rhs, i64 %true, i64 %false) nounwind {
  %condition = icmp ne i64 %lhs, %rhs
  %result = select i1 %condition, i64 %true, i64 %false
  ret i64 %result
}

; CHECK-LABEL: select_true_immediate:
; CHECK:       CSNZ r233, {{r[0-9]+}}, 17
; CHECK-NEXT:  OR r231, r233, 0
; CHECK-NEXT:  POP 0, 0
define i64 @select_true_immediate(i64 %lhs, i64 %rhs, i64 %false) nounwind {
  %condition = icmp slt i64 %lhs, %rhs
  %result = select i1 %condition, i64 17, i64 %false
  ret i64 %result
}

; CHECK-LABEL: select_false_immediate:
; CHECK:       CSZ r233, {{r[0-9]+}}, 23
; CHECK-NEXT:  OR r231, r233, 0
; CHECK-NEXT:  POP 0, 0
define i64 @select_false_immediate(i64 %lhs, i64 %rhs, i64 %true) nounwind {
  %condition = icmp uge i64 %lhs, %rhs
  %result = select i1 %condition, i64 %true, i64 23
  ret i64 %result
}

; Pointer-valued selects use the same tied conditional-set operation.
; CHECK-LABEL: select_pointer:
; CHECK:       AND [[COND:r[0-9]+]], r231, 1
; CHECK-NEXT:  CSNZ r233, [[COND]], r232
; CHECK-NEXT:  OR r231, r233, 0
; CHECK-NEXT:  POP 0, 0
define ptr @select_pointer(i1 %condition, ptr %true, ptr %false) nounwind {
  %result = select i1 %condition, ptr %true, ptr %false
  ret ptr %result
}

; Boundary comparisons must not replace unsigned ordering with signed CMP.
; CHECK-LABEL: compare_unsigned_boundary:
; CHECK:       SRU r231, r231, 63
; CHECK-NEXT:  POP 0, 0
define i1 @compare_unsigned_boundary(i64 %value) nounwind {
  %result = icmp uge i64 %value, -9223372036854775808
  ret i1 %result
}
