; RUN: llc -mtriple=mmix -verify-machineinstrs -filetype=asm %s -o - | FileCheck %s

target triple = "mmix"

; Binary32 call values remain raw low-tetra encodings. Each operation stores
; its inputs with STTU, extends them exactly with LDSF, performs the binary64
; operation, and rounds the result once with STSF before returning its low
; tetra with LDTU.

; CHECK-LABEL: add_f32:
; CHECK-DAG:   STTU r231, r254,
; CHECK-DAG:   STTU r232, r254,
; CHECK-COUNT-2: LDSF
; CHECK:       FADD [[RESULT:r[0-9]+]], {{r[0-9]+}}, {{r[0-9]+}}
; CHECK:       STSF [[RESULT]], r254,
; CHECK:       LDTU r231, r254,
define float @add_f32(float %lhs, float %rhs) {
  %result = fadd float %lhs, %rhs
  ret float %result
}

; CHECK-LABEL: subtract_f32:
; CHECK:       FSUB
; CHECK:       STSF
; CHECK:       LDTU r231
define float @subtract_f32(float %lhs, float %rhs) {
  %result = fsub float %lhs, %rhs
  ret float %result
}

; CHECK-LABEL: multiply_f32:
; CHECK:       FMUL
; CHECK:       STSF
; CHECK:       LDTU r231
define float @multiply_f32(float %lhs, float %rhs) {
  %result = fmul float %lhs, %rhs
  ret float %result
}

; CHECK-LABEL: divide_f32:
; CHECK:       FDIV
; CHECK:       STSF
; CHECK:       LDTU r231
define float @divide_f32(float %lhs, float %rhs) {
  %result = fdiv float %lhs, %rhs
  ret float %result
}

; CHECK-LABEL: square_root_f32:
; CHECK:       FSQRT {{r[0-9]+}}, 4, {{r[0-9]+}}
; CHECK:       STSF
; CHECK:       LDTU r231
define float @square_root_f32(float %value) {
  %result = call float @llvm.sqrt.f32(float %value)
  ret float %result
}

; Quiet equality implements the required equality of positive and negative
; zero after exact extension.
; CHECK-LABEL: equal_f32:
; CHECK:       FEQL r231
define i1 @equal_f32(float %lhs, float %rhs) {
  %result = fcmp oeq float %lhs, %rhs
  ret i1 %result
}

; Ordered inclusive comparison retains the explicit unordered guard for NaNs.
; CHECK-LABEL: ordered_greater_equal_f32:
; CHECK-DAG:   FUN
; CHECK-DAG:   FCMP
define i1 @ordered_greater_equal_f32(float %lhs, float %rhs) {
  %result = fcmp oge float %lhs, %rhs
  ret i1 %result
}

; Ordinary finite constants are extended exactly before the operation.
; CHECK-LABEL: add_finite_f32:
; CHECK:       FADD
; CHECK:       STSF
define float @add_finite_f32(float %value) {
  %result = fadd float %value, 1.5
  ret float %result
}

; CHECK-LABEL: multiply_negative_zero_f32:
; CHECK:       FMUL
; CHECK:       STSF
define float @multiply_negative_zero_f32(float %value) {
  %result = fmul float %value, -0.0
  ret float %result
}

; CHECK-LABEL: add_infinity_f32:
; CHECK:       FADD
; CHECK:       STSF
define float @add_infinity_f32(float %value) {
  %result = fadd float %value, 0x7FF0000000000000
  ret float %result
}

; CHECK-LABEL: add_nan_f32:
; CHECK:       FADD
; CHECK:       STSF
define float @add_nan_f32(float %value) {
  %result = fadd float %value, 0x7FF8000000000000
  ret float %result
}

declare float @llvm.sqrt.f32(float)
