; RUN: llc -mtriple=mmix -verify-machineinstrs < %s | FileCheck %s

target triple = "mmix"

; A shift by one through four followed by addition is exactly one of MMIX's
; nontrapping scaled additions. Both operations use modulo-64-bit semantics.
define i64 @scaled_add_2(i64 %scaled, i64 %addend) {
; CHECK-LABEL: scaled_add_2:
; CHECK:       2ADDU r231, r231, r232
; CHECK-NOT:   SLU
; CHECK-NOT:   ADDU
  %shifted = shl i64 %scaled, 1
  %result = add i64 %shifted, %addend
  ret i64 %result
}

define i64 @scaled_add_4_immediate(i64 %scaled) {
; CHECK-LABEL: scaled_add_4_immediate:
; CHECK:       4ADDU r231, r231, 37
; CHECK-NOT:   SLU
; CHECK-NOT:   ADDU
  %shifted = shl i64 %scaled, 2
  %result = add i64 %shifted, 37
  ret i64 %result
}

define i64 @scaled_add_8_commuted(i64 %addend, i64 %scaled) {
; CHECK-LABEL: scaled_add_8_commuted:
; CHECK:       8ADDU r231, r232, r231
; CHECK-NOT:   SLU
; CHECK-NOT:   ADDU
  %shifted = shl i64 %scaled, 3
  %result = add i64 %addend, %shifted
  ret i64 %result
}

define i64 @scaled_add_16_nsw(i64 %scaled, i64 %addend) {
; CHECK-LABEL: scaled_add_16_nsw:
; CHECK:       16ADDU r231, r231, r232
; CHECK-NOT:   SL{{[^U]}}
; CHECK-NOT:   ADD{{[^U]}}
  %shifted = shl nsw i64 %scaled, 4
  %result = add nsw i64 %shifted, %addend
  ret i64 %result
}

; SelectionDAG has already reduced this inbounds-free GEP to ordinary target
; pointer arithmetic. The scaled add preserves its modulo address value.
define ptr @scaled_gep(ptr %base, i64 %index) {
; CHECK-LABEL: scaled_gep:
; CHECK:       8ADDU r231, r232, r231
  %result = getelementptr i64, ptr %base, i64 %index
  ret ptr %result
}

; The immediate scaled-add form has an unsigned 8-bit addend. Larger constants
; must use the register form without truncation.
define i64 @scaled_add_large_constant(i64 %scaled) {
; CHECK-LABEL: scaled_add_large_constant:
; CHECK:       SETL [[ADDEND:r[0-9]+]], 256
; CHECK-NEXT:  2ADDU r231, r231, [[ADDEND]]
; CHECK-NOT:   2ADDU r231, r231, 0
  %shifted = shl i64 %scaled, 1
  %result = add i64 %shifted, 256
  ret i64 %result
}

; There is no scaled-add instruction for a factor of 32. In particular, nsw
; poison semantics do not authorize the trapping SL or ADD instructions.
define i64 @unsupported_scale(i64 %scaled, i64 %addend) {
; CHECK-LABEL: unsupported_scale:
; CHECK:       SLU
; CHECK:       ADDU
; CHECK-NOT:   16ADDU
; CHECK-NOT:   SL {{.*}}
; CHECK-NOT:   ADD {{.*}}
  %shifted = shl nsw i64 %scaled, 5
  %result = add nsw i64 %shifted, %addend
  ret i64 %result
}

; SADD with a zero second operand is exactly population count.
define i64 @population_count(i64 %value) {
; CHECK-LABEL: population_count:
; CHECK:       SADD r231, r231, 0
  %count = call i64 @llvm.ctpop.i64(i64 %value)
  ret i64 %count
}

; ODIF is scalar unsigned saturating subtraction. Its immediate form is used
; only when the complete unsigned operand fits the architectural Z byte.
define i64 @unsigned_saturating_sub(i64 %lhs, i64 %rhs) {
; CHECK-LABEL: unsigned_saturating_sub:
; CHECK:       ODIF r231, r231, r232
  %result = call i64 @llvm.usub.sat.i64(i64 %lhs, i64 %rhs)
  ret i64 %result
}

define i64 @unsigned_saturating_sub_immediate(i64 %lhs) {
; CHECK-LABEL: unsigned_saturating_sub_immediate:
; CHECK:       ODIF r231, r231, 255
  %result = call i64 @llvm.usub.sat.i64(i64 %lhs, i64 255)
  ret i64 %result
}

; An ordinary scalar bit-select has no relation to MMIX MUX unless rM is known
; to contain the mask. It also has no Boolean-matrix or packed-lane semantics.
define i64 @ordinary_bit_select(i64 %lhs, i64 %rhs, i64 %mask) {
; CHECK-LABEL: ordinary_bit_select:
; CHECK-NOT:   MUX
; CHECK-NOT:   MOR
; CHECK-NOT:   MXOR
; CHECK-NOT:   BDIF
; CHECK-NOT:   WDIF
; CHECK-NOT:   TDIF
; CHECK:       POP 0, 0
  %lhs.bits = and i64 %lhs, %mask
  %inverted = xor i64 %mask, -1
  %rhs.bits = and i64 %rhs, %inverted
  %result = or i64 %lhs.bits, %rhs.bits
  ret i64 %result
}

; A generic vector's lane numbering is not by itself proof that its lanes map
; to MMIX's big-endian byte positions. Keep the reviewed scalarized lowering
; until the backend has an explicit packed-lane contract.
define i64 @packed_byte_saturating_sub(i64 %lhs, i64 %rhs) {
; CHECK-LABEL: packed_byte_saturating_sub:
; CHECK-NOT:   BDIF
; CHECK-NOT:   WDIF
; CHECK-NOT:   TDIF
; CHECK:       POP 0, 0
  %lhs.vector = bitcast i64 %lhs to <8 x i8>
  %rhs.vector = bitcast i64 %rhs to <8 x i8>
  %difference = call <8 x i8> @llvm.usub.sat.v8i8(
      <8 x i8> %lhs.vector, <8 x i8> %rhs.vector)
  %result = bitcast <8 x i8> %difference to i64
  ret i64 %result
}

declare i64 @llvm.ctpop.i64(i64)
declare i64 @llvm.usub.sat.i64(i64, i64)
declare <8 x i8> @llvm.usub.sat.v8i8(<8 x i8>, <8 x i8>)
