; RUN: llc -mtriple=mmix -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=mmix -O1 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=mmix -O2 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=mmix -O3 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=mmix -O2 -verify-machineinstrs %s -o - | FileCheck %s

target triple = "mmix"

; Narrow scalar integer values promote to i64 registers while retaining their
; ABI extension and truncation semantics.
define zeroext i8 @narrow_add(i8 zeroext %lhs, i8 zeroext %rhs) {
; CHECK-LABEL: narrow_add:
; CHECK:       ADDU
; CHECK:       AND
; CHECK:       POP 0, 0
  %result = add i8 %lhs, %rhs
  ret i8 %result
}

; Internal wide integer operations split into reviewed i64 operations. Wide
; values remain unsupported at the provisional ABI boundary.
define i64 @wide_add_high_half(i64 %low, i64 %high) {
; CHECK-LABEL: wide_add_high_half:
; CHECK-DAG:   CMPU
; CHECK-DAG:   ADDU
; CHECK:       POP 0, 0
  %low.wide = zext i64 %low to i128
  %high.wide = zext i64 %high to i128
  %shifted = shl i128 %high.wide, 64
  %value = or i128 %shifted, %low.wide
  %sum = add i128 %value, 1
  %sum.high = lshr i128 %sum, 64
  %result = trunc i128 %sum.high to i64
  ret i64 %result
}

; Fixed vectors have no native register class and are deliberately scalarized.
define i64 @fixed_vector_add(i64 %packed) {
; CHECK-LABEL: fixed_vector_add:
; CHECK-NOT:   BDIF
; CHECK:       ADDU
; CHECK:       POP 0, 0
  %vector = bitcast i64 %packed to <2 x i32>
  %sum = add <2 x i32> %vector, <i32 1, i32 2>
  %result = bitcast <2 x i32> %sum to i64
  ret i64 %result
}
