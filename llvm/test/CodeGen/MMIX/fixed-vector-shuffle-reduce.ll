; RUN: llc -mtriple=mmix -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=mmix -O2 -verify-machineinstrs %s -o - | FileCheck %s

target triple = "mmix"

declare i32 @llvm.vector.reduce.add.v2i32(<2 x i32>)
declare i32 @llvm.vector.reduce.mul.v2i32(<2 x i32>)
declare i32 @llvm.vector.reduce.and.v2i32(<2 x i32>)
declare i32 @llvm.vector.reduce.or.v2i32(<2 x i32>)
declare i32 @llvm.vector.reduce.xor.v2i32(<2 x i32>)
declare i1 @llvm.vector.reduce.and.v2i1(<2 x i1>)
declare float @llvm.vector.reduce.fadd.v2f32(float, <2 x float>)

; Constant shuffles use scalar lane extraction and construction. No native
; vector shuffle instruction or runtime interface is introduced.
; CHECK-LABEL: splat_word0:
; CHECK-NOT:   PUSHJ
; CHECK:       OR
; CHECK:       POP 0, 0
define <2 x i32> @splat_word0(<2 x i32> %value) {
  %result = shufflevector <2 x i32> %value, <2 x i32> poison,
                          <2 x i32> zeroinitializer
  ret <2 x i32> %result
}

; CHECK-LABEL: reverse_bytes:
; CHECK-NOT:   PUSHJ
; CHECK:       SRU
; CHECK:       SLU
; CHECK:       POP 0, 0
define <8 x i8> @reverse_bytes(<8 x i8> %value) {
  %result = shufflevector <8 x i8> %value, <8 x i8> poison,
                          <8 x i32> <i32 7, i32 6, i32 5, i32 4,
                                      i32 3, i32 2, i32 1, i32 0>
  ret <8 x i8> %result
}

; CHECK-LABEL: concatenate_halves:
; CHECK-NOT:   PUSHJ
; CHECK:       SLU
; CHECK:       OR
; CHECK:       POP 0, 0
define <4 x i16> @concatenate_halves(<2 x i16> %lhs, <2 x i16> %rhs) {
  %result = shufflevector <2 x i16> %lhs, <2 x i16> %rhs,
                          <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  ret <4 x i16> %result
}

; CHECK-LABEL: extract_middle_halves:
; CHECK-NOT:   PUSHJ
; CHECK:       SRU
; CHECK:       POP 0, 0
define <2 x i16> @extract_middle_halves(<4 x i16> %value) {
  %result = shufflevector <4 x i16> %value, <4 x i16> poison,
                          <2 x i32> <i32 1, i32 2>
  ret <2 x i16> %result
}

; CHECK-LABEL: reduce_add_words:
; CHECK-NOT:   PUSHJ
; CHECK:       ADDU
; CHECK:       POP 0, 0
define i32 @reduce_add_words(<2 x i32> %value) {
  %result = call i32 @llvm.vector.reduce.add.v2i32(<2 x i32> %value)
  ret i32 %result
}

; CHECK-LABEL: reduce_multiply_words:
; CHECK-NOT:   PUSHJ
; CHECK:       MULU
; CHECK:       POP 0, 0
define i32 @reduce_multiply_words(<2 x i32> %value) {
  %result = call i32 @llvm.vector.reduce.mul.v2i32(<2 x i32> %value)
  ret i32 %result
}

; CHECK-LABEL: reduce_and_words:
; CHECK-NOT:   PUSHJ
; CHECK:       AND
; CHECK:       POP 0, 0
define i32 @reduce_and_words(<2 x i32> %value) {
  %result = call i32 @llvm.vector.reduce.and.v2i32(<2 x i32> %value)
  ret i32 %result
}

; CHECK-LABEL: reduce_or_words:
; CHECK-NOT:   PUSHJ
; CHECK:       OR
; CHECK:       POP 0, 0
define i32 @reduce_or_words(<2 x i32> %value) {
  %result = call i32 @llvm.vector.reduce.or.v2i32(<2 x i32> %value)
  ret i32 %result
}

; CHECK-LABEL: reduce_xor_words:
; CHECK-NOT:   PUSHJ
; CHECK:       XOR
; CHECK:       POP 0, 0
define i32 @reduce_xor_words(<2 x i32> %value) {
  %result = call i32 @llvm.vector.reduce.xor.v2i32(<2 x i32> %value)
  ret i32 %result
}

; Comparison reduction preserves the per-lane predicate before reducing it.
; CHECK-LABEL: all_words_less:
; CHECK-NOT:   PUSHJ
; CHECK-COUNT-3: CMPU
; CHECK:       ZSZ
; CHECK:       POP 0, 0
define i1 @all_words_less(<2 x i32> %lhs, <2 x i32> %rhs) {
  %comparison = icmp ult <2 x i32> %lhs, %rhs
  %result = call i1 @llvm.vector.reduce.and.v2i1(<2 x i1> %comparison)
  ret i1 %result
}

; Ordered floating reduction keeps the explicit start value and lane order.
; CHECK-LABEL: reduce_ordered_floats:
; CHECK-NOT:   PUSHJ
; CHECK-COUNT-2: FADD
; CHECK:       POP 0, 0
define float @reduce_ordered_floats(float %start, <2 x float> %value) {
  %result = call float @llvm.vector.reduce.fadd.v2f32(
      float %start, <2 x float> %value)
  ret float %result
}
