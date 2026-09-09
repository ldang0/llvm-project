; RUN: llc -mtriple=mmix -verify-machineinstrs -filetype=asm %s -o - | FileCheck %s
; RUN: llc -mtriple=mmix -verify-machineinstrs -stop-after=mmix-isel %s -o - | FileCheck %s --check-prefix=MIR

target triple = "mmix"

; CHECK-LABEL: multiply:
; CHECK:       MULU r231, r231, r232
; CHECK-NEXT:  POP 0, 0
define i64 @multiply(i64 %lhs, i64 %rhs) nounwind {
  %result = mul i64 %lhs, %rhs
  ret i64 %result
}

; CHECK-LABEL: multiply_immediate:
; CHECK:       MULU r231, r231, 255
; CHECK-NEXT:  POP 0, 0
define i64 @multiply_immediate(i64 %value) nounwind {
  %result = mul i64 %value, 255
  ret i64 %result
}

; nsw poison does not permit MMIX's trapping MUL instruction.
; CHECK-LABEL: multiply_nsw:
; CHECK:       MULU r231, r231, r232
; CHECK-NEXT:  POP 0, 0
define i64 @multiply_nsw(i64 %lhs, i64 %rhs) nounwind {
  %result = mul nsw i64 %lhs, %rhs
  ret i64 %result
}

; CHECK-LABEL: multiply_high_unsigned:
; CHECK:       MULU [[LOW:r[0-9]+]], r231, r232
; CHECK-NEXT:  GET r231, rH
; CHECK-NEXT:  POP 0, 0
; MIR-LABEL:  name: multiply_high_unsigned
; MIR:        MULU {{.*}}implicit-def $rh
; MIR-NEXT:   %{{[0-9]+}}:fpr64codegen = GET $rh
define i64 @multiply_high_unsigned(i64 %lhs, i64 %rhs) nounwind {
  %lhs.wide = zext i64 %lhs to i128
  %rhs.wide = zext i64 %rhs to i128
  %product = mul i128 %lhs.wide, %rhs.wide
  %high.wide = lshr i128 %product, 64
  %high = trunc i128 %high.wide to i64
  ret i64 %high
}

; Signed high multiplication corrects the unsigned high half with operand sign
; masks, without selecting trapping MUL.
; CHECK-LABEL: multiply_high_signed:
; CHECK:       SR [[LHSMASK:r[0-9]+]], r231, 63
; CHECK:       MULU [[SIGNEDLOW:r[0-9]+]], r231, r232
; CHECK-NEXT:  GET [[UNSIGNEDHIGH:r[0-9]+]], rH
; CHECK:       SUBU r231,
; CHECK-NEXT:  POP 0, 0
define i64 @multiply_high_signed(i64 %lhs, i64 %rhs) nounwind {
  %lhs.wide = sext i64 %lhs to i128
  %rhs.wide = sext i64 %rhs to i128
  %product = mul i128 %lhs.wide, %rhs.wide
  %high.wide = lshr i128 %product, 64
  %high = trunc i128 %high.wide to i64
  ret i64 %high
}

; The low and high halves share one architectural multiplication.
; CHECK-LABEL: multiply_low_high_unsigned:
; CHECK:       MULU [[PAIRLOW:r[0-9]+]], r231, r232
; CHECK-NEXT:  GET [[PAIRHIGH:r[0-9]+]], rH
; CHECK-NEXT:  XOR r231, [[PAIRLOW]], [[PAIRHIGH]]
; CHECK-NEXT:  POP 0, 0
define i64 @multiply_low_high_unsigned(i64 %lhs, i64 %rhs) nounwind {
  %lhs.wide = zext i64 %lhs to i128
  %rhs.wide = zext i64 %rhs to i128
  %product = mul i128 %lhs.wide, %rhs.wide
  %low = trunc i128 %product to i64
  %high.wide = lshr i128 %product, 64
  %high = trunc i128 %high.wide to i64
  %result = xor i64 %low, %high
  ret i64 %result
}

; CHECK-LABEL: divide_unsigned:
; CHECK:       PUT rD, 0
; CHECK-NEXT:  DIVU r231, r231, r232
; CHECK-NEXT:  POP 0, 0
; MIR-LABEL:  name: divide_unsigned
; MIR:        SET_RD_ZERO implicit-def $rd
; MIR-NEXT:   %{{[0-9]+}}:fpr64codegen = DIVU {{.*}}implicit-def dead $rr, implicit $rd
define i64 @divide_unsigned(i64 %dividend, i64 %divisor) nounwind {
  %quotient = udiv i64 %dividend, %divisor
  ret i64 %quotient
}

; CHECK-LABEL: remainder_unsigned:
; CHECK:       PUT rD, 0
; CHECK-NEXT:  DIVU [[UQUOT:r[0-9]+]], r231, r232
; CHECK-NEXT:  GET r231, rR
; CHECK-NEXT:  POP 0, 0
define i64 @remainder_unsigned(i64 %dividend, i64 %divisor) nounwind {
  %remainder = urem i64 %dividend, %divisor
  ret i64 %remainder
}

; The quotient and remainder share one DIVU and its rR result.
; CHECK-LABEL: divide_remainder_unsigned:
; CHECK:       PUT rD, 0
; CHECK-NEXT:  DIVU [[UPAIRQUOT:r[0-9]+]], r231, r232
; CHECK-NEXT:  GET [[UPAIRREM:r[0-9]+]], rR
; CHECK-NEXT:  ADDU r231, [[UPAIRQUOT]], [[UPAIRREM]]
; CHECK-NEXT:  POP 0, 0
; MIR-LABEL:  name: divide_remainder_unsigned
; MIR:        SET_RD_ZERO implicit-def $rd
; MIR-NEXT:   %[[UQM:[0-9]+]]:fpr64codegen = DIVU {{.*}}implicit-def $rr, implicit $rd
; MIR-NEXT:   %[[URM:[0-9]+]]:fpr64codegen = GET $rr
define i64 @divide_remainder_unsigned(i64 %dividend, i64 %divisor) nounwind {
  %quotient = udiv i64 %dividend, %divisor
  %remainder = urem i64 %dividend, %divisor
  %result = add i64 %quotient, %remainder
  ret i64 %result
}

; Constant division may use the high-half multiply result.
; CHECK-LABEL: divide_unsigned_immediate:
; CHECK:       MULU
; CHECK-NEXT:  GET {{.*}}, rH
; CHECK-NEXT:  SRU r231,
; CHECK-NEXT:  POP 0, 0
define i64 @divide_unsigned_immediate(i64 %dividend) nounwind {
  %quotient = udiv i64 %dividend, 255
  ret i64 %quotient
}

; MMIX DIV rounds toward negative infinity. The following arithmetic uses the
; rR result to increment a nonexact quotient when operand signs differ.
; CHECK-LABEL: divide_signed:
; CHECK:       DIV [[FLOORQ:r[0-9]+]], r231, r232
; CHECK-NEXT:  GET [[FLOORR:r[0-9]+]], rR
; CHECK:       XOR
; CHECK:       SR [[CORRECTION:r[0-9]+]], {{.*}}, 63
; CHECK-NEXT:  SUBU r231, [[FLOORQ]], [[CORRECTION]]
; CHECK-NEXT:  POP 0, 0
; MIR-LABEL:  name: divide_signed
; MIR:        DIV {{.*}}implicit-def $rr
; MIR-NEXT:   %{{[0-9]+}}:fpr64codegen = GET $rr
define i64 @divide_signed(i64 %dividend, i64 %divisor) nounwind {
  %quotient = sdiv i64 %dividend, %divisor
  ret i64 %quotient
}

; CHECK-LABEL: remainder_signed:
; CHECK:       DIV [[SREMQUOT:r[0-9]+]], r231, r232
; CHECK-NEXT:  GET [[FLOORSREM:r[0-9]+]], rR
; CHECK:       AND {{.*}}, r232,
; CHECK-NEXT:  SUBU r231, [[FLOORSREM]],
; CHECK-NEXT:  POP 0, 0
define i64 @remainder_signed(i64 %dividend, i64 %divisor) nounwind {
  %remainder = srem i64 %dividend, %divisor
  ret i64 %remainder
}

; The corrected quotient and remainder share one DIV.
; CHECK-LABEL: divide_remainder_signed:
; CHECK:       DIV [[SPAIRQUOT:r[0-9]+]], r231, r232
; CHECK-NEXT:  GET [[SPAIRREM:r[0-9]+]], rR
; CHECK-NOT:   DIV
; CHECK:       ADDU r231,
; CHECK-NEXT:  POP 0, 0
define i64 @divide_remainder_signed(i64 %dividend, i64 %divisor) nounwind {
  %quotient = sdiv i64 %dividend, %divisor
  %remainder = srem i64 %dividend, %divisor
  %result = add i64 %quotient, %remainder
  ret i64 %result
}

; CHECK-LABEL: multiply_i32:
; CHECK:       MULU r231, r231, r232
; CHECK-NEXT:  POP 0, 0
define i32 @multiply_i32(i32 %lhs, i32 %rhs) nounwind {
  %result = mul i32 %lhs, %rhs
  ret i32 %result
}

; Narrow unsigned operands are zero-extended before DIVU.
; CHECK-LABEL: divide_unsigned_i32:
; CHECK:       AND
; CHECK:       AND
; CHECK:       PUT rD, 0
; CHECK-NEXT:  DIVU r231,
; CHECK-NEXT:  POP 0, 0
define i32 @divide_unsigned_i32(i32 %dividend, i32 %divisor) nounwind {
  %quotient = udiv i32 %dividend, %divisor
  ret i32 %quotient
}

; Narrow signed operands are sign-extended with nontrapping shifts before DIV.
; CHECK-LABEL: divide_signed_i32:
; CHECK:       SLU
; CHECK-NEXT:  SR
; CHECK:       SLU
; CHECK-NEXT:  SR
; CHECK:       DIV
; CHECK-NEXT:  GET {{.*}}, rR
; CHECK:       POP 0, 0
define i32 @divide_signed_i32(i32 %dividend, i32 %divisor) {
  %quotient = sdiv i32 %dividend, %divisor
  ret i32 %quotient
}
