; RUN: llc -mtriple=mmix -verify-machineinstrs -filetype=asm %s -o - | FileCheck %s

target triple = "mmix"

; Numeric operands use FCMP. The AND of complete zero bit patterns makes +0
; win over -0, and setting f64 fraction bit 51 quiets a signaling NaN.
; CHECK-LABEL: maximum:
; CHECK:       FCMP
; CHECK:       AND
; CHECK:       SETH [[QUIET:r[0-9]+]], 8
; CHECK:       OR {{r[0-9]+}}, {{r[0-9]+}}, [[QUIET]]
; CHECK-NOT:   fmax
; CHECK:       POP 0, 0
define double @maximum(double %lhs, double %rhs) {
  %result = call double @llvm.maxnum.f64(double %lhs, double %rhs)
  ret double %result
}

; The OR of complete zero bit patterns makes -0 win over +0. NaN handling
; shares the same explicit exponent, fraction, and quiet-bit classification.
; CHECK-LABEL: minimum:
; CHECK:       FCMP
; CHECK:       OR
; CHECK:       SETH [[QUIET:r[0-9]+]], 8
; CHECK:       OR {{r[0-9]+}}, {{r[0-9]+}}, [[QUIET]]
; CHECK-NOT:   fmin
; CHECK:       POP 0, 0
define double @minimum(double %lhs, double %rhs) {
  %result = call double @llvm.minnum.f64(double %lhs, double %rhs)
  ret double %result
}

; f32 remains in its raw low-tetra representation while classifying and
; quieting NaNs. Only the ordered comparison converts the operands to f64.
; CHECK-LABEL: maximum_float:
; CHECK:       LDSF
; CHECK:       FCMP
; CHECK:       SETML [[QUIET:r[0-9]+]], 64
; CHECK:       OR {{r[0-9]+}}, {{r[0-9]+}}, [[QUIET]]
; CHECK-NOT:   fmax
; CHECK:       POP 0, 0
define float @maximum_float(float %lhs, float %rhs) {
  %result = call float @llvm.maxnum.f32(float %lhs, float %rhs)
  ret float %result
}

declare double @llvm.maxnum.f64(double, double)
declare double @llvm.minnum.f64(double, double)
declare float @llvm.maxnum.f32(float, float)
