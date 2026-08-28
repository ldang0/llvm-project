; RUN: llc -mtriple=mmix -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=mmix -O2 -verify-machineinstrs %s -o - | FileCheck %s

target triple = "mmix"

; Width changes preserve source lane correspondence and truncate each lane
; independently rather than truncating the packed scalar as a whole.
; CHECK-LABEL: truncate_words_to_halves:
; CHECK-NOT:   PUSHJ
; CHECK:       AND
; CHECK:       POP 0, 0
define <2 x i16> @truncate_words_to_halves(<2 x i32> %value) {
  %result = trunc <2 x i32> %value to <2 x i16>
  ret <2 x i16> %result
}

; CHECK-LABEL: zero_extend_halves_to_words:
; CHECK-NOT:   PUSHJ
; CHECK:       AND
; CHECK:       POP 0, 0
define <2 x i32> @zero_extend_halves_to_words(<2 x i16> %value) {
  %result = zext <2 x i16> %value to <2 x i32>
  ret <2 x i32> %result
}

; CHECK-LABEL: sign_extend_halves_to_words:
; CHECK-NOT:   PUSHJ
; CHECK:       SL
; CHECK:       SR
; CHECK:       POP 0, 0
define <2 x i32> @sign_extend_halves_to_words(<2 x i16> %value) {
  %result = sext <2 x i16> %value to <2 x i32>
  ret <2 x i32> %result
}

; Integer and binary32 conversion reuses the scalar conversion path per lane.
; CHECK-LABEL: signed_words_to_floats:
; CHECK-NOT:   PUSHJ
; CHECK-COUNT-2: FLOT
; CHECK:       POP 0, 0
define <2 x float> @signed_words_to_floats(<2 x i32> %value) {
  %result = sitofp <2 x i32> %value to <2 x float>
  ret <2 x float> %result
}

; CHECK-LABEL: unsigned_words_to_floats:
; CHECK-NOT:   PUSHJ
; CHECK-COUNT-2: FLOTU
; CHECK:       POP 0, 0
define <2 x float> @unsigned_words_to_floats(<2 x i32> %value) {
  %result = uitofp <2 x i32> %value to <2 x float>
  ret <2 x float> %result
}

; CHECK-LABEL: floats_to_signed_words:
; CHECK-NOT:   PUSHJ
; CHECK-COUNT-2: FIX
; CHECK:       POP 0, 0
define <2 x i32> @floats_to_signed_words(<2 x float> %value) {
  %result = fptosi <2 x float> %value to <2 x i32>
  ret <2 x i32> %result
}

; CHECK-LABEL: floats_to_unsigned_words:
; CHECK-NOT:   PUSHJ
; CHECK-COUNT-2: FIXU
; CHECK:       POP 0, 0
define <2 x i32> @floats_to_unsigned_words(<2 x float> %value) {
  %result = fptoui <2 x float> %value to <2 x i32>
  ret <2 x i32> %result
}

; Same-width reinterpretation changes only the lane type. The packed bits are
; returned unchanged, independent of host endianness.
; CHECK-LABEL: reinterpret_words_as_floats:
; CHECK-NOT:   PUSHJ
; CHECK-NOT:   FLOT
; CHECK-NOT:   FIX
; CHECK:       POP 0, 0
define <2 x float> @reinterpret_words_as_floats(<2 x i32> %value) {
  %result = bitcast <2 x i32> %value to <2 x float>
  ret <2 x float> %result
}

; Optimized constant folding retains lane order at signed boundary values.
; CHECK-LABEL: constant_extend_halves:
; CHECK:       SETMH r231, 1
; CHECK:       NEGU r231, 0, r231
; CHECK:       POP 0, 0
define <2 x i32> @constant_extend_halves() {
  %result = sext <2 x i16> <i16 -1, i16 0> to <2 x i32>
  ret <2 x i32> %result
}
