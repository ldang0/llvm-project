; RUN: llc -mtriple=mmix -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=mmix -O2 -verify-machineinstrs %s -o - | FileCheck %s

target triple = "mmix"

; Binary32 lanes reuse the scalar MMIX floating path independently. The
; packed transport remains integer bits at the function boundary.
; CHECK-LABEL: add_floats:
; CHECK-NOT:   PUSHJ
; CHECK-COUNT-2: FADD
; CHECK:       POP 0, 0
define <2 x float> @add_floats(<2 x float> %lhs, <2 x float> %rhs) {
  %result = fadd <2 x float> %lhs, %rhs
  ret <2 x float> %result
}

; CHECK-LABEL: subtract_floats:
; CHECK-NOT:   PUSHJ
; CHECK-COUNT-2: FSUB
; CHECK:       POP 0, 0
define <2 x float> @subtract_floats(<2 x float> %lhs, <2 x float> %rhs) {
  %result = fsub <2 x float> %lhs, %rhs
  ret <2 x float> %result
}

; CHECK-LABEL: multiply_floats:
; CHECK-NOT:   PUSHJ
; CHECK-COUNT-2: FMUL
; CHECK:       POP 0, 0
define <2 x float> @multiply_floats(<2 x float> %lhs, <2 x float> %rhs) {
  %result = fmul <2 x float> %lhs, %rhs
  ret <2 x float> %result
}

; CHECK-LABEL: divide_floats:
; CHECK-NOT:   PUSHJ
; CHECK-COUNT-2: FDIV
; CHECK:       POP 0, 0
define <2 x float> @divide_floats(<2 x float> %lhs, <2 x float> %rhs) {
  %result = fdiv <2 x float> %lhs, %rhs
  ret <2 x float> %result
}

; A one-lane binary64 vector follows the same scalar operation without a
; vector-specific runtime ABI.
; CHECK-LABEL: fused_double_arithmetic:
; CHECK-NOT:   PUSHJ
; CHECK:       FMUL
; CHECK:       FADD
; CHECK:       POP 0, 0
define <1 x double> @fused_double_arithmetic(<1 x double> %lhs,
                                             <1 x double> %rhs,
                                             <1 x double> %addend) {
  %product = fmul <1 x double> %lhs, %rhs
  %result = fadd <1 x double> %product, %addend
  ret <1 x double> %result
}

; Fast-math flags survive scalarization and do not change packed lane order.
; CHECK-LABEL: add_floats_fast:
; CHECK-NOT:   PUSHJ
; CHECK-COUNT-2: FADD
; CHECK:       POP 0, 0
define <2 x float> @add_floats_fast(<2 x float> %lhs, <2 x float> %rhs) {
  %result = fadd fast <2 x float> %lhs, %rhs
  ret <2 x float> %result
}
