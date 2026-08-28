; RUN: llc -mtriple=mmix -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=mmix -O2 -verify-machineinstrs %s -o - | FileCheck %s

target triple = "mmix"

; Integer vectors are scalarized lane by lane. Narrow arithmetic therefore
; wraps independently instead of carrying between adjacent packed lanes.
; CHECK-LABEL: add_bytes:
; CHECK-NOT:   PUSHJ
; CHECK:       ADDU
; CHECK:       AND
; CHECK:       POP 0, 0
define <8 x i8> @add_bytes(<8 x i8> %lhs, <8 x i8> %rhs) {
  %result = add <8 x i8> %lhs, %rhs
  ret <8 x i8> %result
}

; CHECK-LABEL: sub_halves:
; CHECK-NOT:   PUSHJ
; CHECK:       SUBU
; CHECK:       POP 0, 0
define <2 x i16> @sub_halves(<2 x i16> %lhs, <2 x i16> %rhs) {
  %result = sub <2 x i16> %lhs, %rhs
  ret <2 x i16> %result
}

; CHECK-LABEL: multiply_words:
; CHECK-NOT:   PUSHJ
; CHECK:       MULU
; CHECK:       POP 0, 0
define <2 x i32> @multiply_words(<2 x i32> %lhs, <2 x i32> %rhs) {
  %result = mul <2 x i32> %lhs, %rhs
  ret <2 x i32> %result
}

; CHECK-LABEL: negate_words:
; CHECK-NOT:   PUSHJ
; CHECK:       NEG
; CHECK:       POP 0, 0
define <2 x i32> @negate_words(<2 x i32> %value) {
  %result = sub <2 x i32> zeroinitializer, %value
  ret <2 x i32> %result
}

; CHECK-LABEL: and_words:
; CHECK-NOT:   PUSHJ
; CHECK:       AND
; CHECK:       POP 0, 0
define <2 x i32> @and_words(<2 x i32> %lhs, <2 x i32> %rhs) {
  %result = and <2 x i32> %lhs, %rhs
  ret <2 x i32> %result
}

; CHECK-LABEL: or_words:
; CHECK-NOT:   PUSHJ
; CHECK:       OR
; CHECK:       POP 0, 0
define <2 x i32> @or_words(<2 x i32> %lhs, <2 x i32> %rhs) {
  %result = or <2 x i32> %lhs, %rhs
  ret <2 x i32> %result
}

; CHECK-LABEL: xor_words:
; CHECK-NOT:   PUSHJ
; CHECK:       XOR
; CHECK:       POP 0, 0
define <2 x i32> @xor_words(<2 x i32> %lhs, <2 x i32> %rhs) {
  %result = xor <2 x i32> %lhs, %rhs
  ret <2 x i32> %result
}

; Variable shift counts apply independently to each lane.
; CHECK-LABEL: shift_words:
; CHECK-NOT:   PUSHJ
; CHECK:       SLU
; CHECK:       SRU
; CHECK:       SR
; CHECK:       POP 0, 0
define <2 x i32> @shift_words(<2 x i32> %value, <2 x i32> %amount) {
  %left = shl <2 x i32> %value, %amount
  %logical = lshr <2 x i32> %left, %amount
  %arithmetic = ashr <2 x i32> %logical, %amount
  ret <2 x i32> %arithmetic
}

; Constant operands retain lane-local wrapping as well.
; CHECK-LABEL: add_constant_halves:
; CHECK-NOT:   PUSHJ
; CHECK:       ADDU
; CHECK:       POP 0, 0
define <2 x i16> @add_constant_halves(<2 x i16> %value) {
  %result = add <2 x i16> %value, <i16 1, i16 -1>
  ret <2 x i16> %result
}

; Contracted mask vectors use one zero-or-one bit per source lane. Boolean
; operations must not leak into neighboring packed lanes.
; CHECK-LABEL: and_mask:
; CHECK-NOT:   PUSHJ
; CHECK:       AND
; CHECK:       POP 0, 0
define <8 x i1> @and_mask(<8 x i1> %lhs, <8 x i1> %rhs) {
  %result = and <8 x i1> %lhs, %rhs
  ret <8 x i1> %result
}

; CHECK-LABEL: or_mask:
; CHECK-NOT:   PUSHJ
; CHECK:       OR
; CHECK:       POP 0, 0
define <8 x i1> @or_mask(<8 x i1> %lhs, <8 x i1> %rhs) {
  %result = or <8 x i1> %lhs, %rhs
  ret <8 x i1> %result
}

; CHECK-LABEL: xor_mask:
; CHECK-NOT:   PUSHJ
; CHECK:       XOR
; CHECK:       POP 0, 0
define <8 x i1> @xor_mask(<8 x i1> %lhs, <8 x i1> %rhs) {
  %result = xor <8 x i1> %lhs, %rhs
  ret <8 x i1> %result
}
