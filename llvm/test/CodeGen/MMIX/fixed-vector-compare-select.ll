; RUN: llc -mtriple=mmix -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=mmix -O2 -verify-machineinstrs %s -o - | FileCheck %s

target triple = "mmix"

; Signed and unsigned predicates are evaluated per lane and packed back into
; the contracted mask representation.
; CHECK-LABEL: compare_signed_words:
; CHECK-NOT:   PUSHJ
; CHECK:       CMP
; CHECK:       POP 0, 0
define i64 @compare_signed_words(<2 x i32> %lhs, <2 x i32> %rhs) {
  %result = icmp slt <2 x i32> %lhs, %rhs
  %packed = bitcast <2 x i1> %result to i2
  %extended = zext i2 %packed to i64
  ret i64 %extended
}

; CHECK-LABEL: compare_unsigned_words:
; CHECK-NOT:   PUSHJ
; CHECK:       CMPU
; CHECK:       POP 0, 0
define i64 @compare_unsigned_words(<2 x i32> %lhs, <2 x i32> %rhs) {
  %result = icmp uge <2 x i32> %lhs, %rhs
  %packed = bitcast <2 x i1> %result to i2
  %extended = zext i2 %packed to i64
  ret i64 %extended
}

; Ordered comparison reuses scalar binary32 comparison lowering for each lane.
; CHECK-LABEL: compare_ordered_floats:
; CHECK-NOT:   PUSHJ
; CHECK:       FCMP
; CHECK:       POP 0, 0
define i64 @compare_ordered_floats(<2 x float> %lhs, <2 x float> %rhs) {
  %result = fcmp olt <2 x float> %lhs, %rhs
  %packed = bitcast <2 x i1> %result to i2
  %extended = zext i2 %packed to i64
  ret i64 %extended
}

; Unordered predicates retain scalar MMIX NaN detection rather than being
; folded into an ordered packed comparison.
; CHECK-LABEL: compare_unordered_floats:
; CHECK-NOT:   PUSHJ
; CHECK:       FUN
; CHECK:       POP 0, 0
define i64 @compare_unordered_floats(<2 x float> %lhs, <2 x float> %rhs) {
  %result = fcmp uno <2 x float> %lhs, %rhs
  %packed = bitcast <2 x i1> %result to i2
  %extended = zext i2 %packed to i64
  ret i64 %extended
}

; A mixed mask must select each integer lane independently. Treating its
; packed bits as one scalar condition would return only one input vector.
; CHECK-LABEL: select_words:
; CHECK-NOT:   PUSHJ
; CHECK:       CS
; CHECK:       CS
; CHECK:       POP 0, 0
define <2 x i32> @select_words(i64 %packed_condition,
                               <2 x i32> %when_true,
                               <2 x i32> %when_false) {
  %narrow_condition = trunc i64 %packed_condition to i2
  %condition = bitcast i2 %narrow_condition to <2 x i1>
  %result = select <2 x i1> %condition, <2 x i32> %when_true,
                                      <2 x i32> %when_false
  ret <2 x i32> %result
}

; Floating selection is bit-preserving and does not introduce arithmetic or a
; vector-specific runtime boundary.
; CHECK-LABEL: select_floats:
; CHECK-NOT:   PUSHJ
; CHECK:       CS
; CHECK:       CS
; CHECK:       POP 0, 0
define <2 x float> @select_floats(i64 %packed_condition,
                                  <2 x float> %when_true,
                                  <2 x float> %when_false) {
  %narrow_condition = trunc i64 %packed_condition to i2
  %condition = bitcast i2 %narrow_condition to <2 x i1>
  %result = select <2 x i1> %condition, <2 x float> %when_true,
                                      <2 x float> %when_false
  ret <2 x float> %result
}
