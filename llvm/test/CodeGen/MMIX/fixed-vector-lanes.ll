; RUN: llc -mtriple=mmix -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=mmix -O2 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=mmix -O2 -verify-machineinstrs %s -o - | FileCheck %s

target triple = "mmix"

; Lane zero occupies the most-significant lane bits of the packed scalar.
; CHECK-LABEL: constant_words:
; CHECK:       SETL r231, 30600
; CHECK:       INCML r231, 21862
; CHECK:       INCMH r231, 13124
; CHECK:       INCH r231, 4386
define i64 @constant_words() {
  %bits = bitcast <2 x i32> <i32 287454020, i32 1432778632> to i64
  ret i64 %bits
}

; Mask lane zero likewise occupies the most-significant packed bit.
; CHECK-LABEL: constant_mask:
; CHECK:       SETL r231, 165
define i8 @constant_mask() {
  %bits = bitcast <8 x i1> <i1 true, i1 false, i1 true, i1 false,
                                i1 false, i1 true, i1 false, i1 true> to i8
  ret i8 %bits
}

; CHECK-LABEL: build_words:
; CHECK:       SLU [[HIGH:r[0-9]+]], r231, 32
; CHECK:       OR r231, {{r[0-9]+}}, [[HIGH]]
define i64 @build_words(i32 %lane0, i32 %lane1) {
  %v0 = insertelement <2 x i32> poison, i32 %lane0, i64 0
  %v1 = insertelement <2 x i32> %v0, i32 %lane1, i64 1
  %bits = bitcast <2 x i32> %v1 to i64
  ret i64 %bits
}

; Floating lanes use the same bit-preserving placement.
; CHECK-LABEL: build_floats:
; CHECK:       SLU [[HIGH:r[0-9]+]], r231, 32
; CHECK:       OR r231, {{r[0-9]+}}, [[HIGH]]
define i64 @build_floats(float %lane0, float %lane1) {
  %v0 = insertelement <2 x float> poison, float %lane0, i64 0
  %v1 = insertelement <2 x float> %v0, float %lane1, i64 1
  %bits = bitcast <2 x float> %v1 to i64
  ret i64 %bits
}

; The remaining contracted element and width forms use the same scalarization
; path. The compile-only runs above retain this construction matrix.
define i8 @build_byte(i8 %lane0) {
  %vector = insertelement <1 x i8> poison, i8 %lane0, i64 0
  %bits = bitcast <1 x i8> %vector to i8
  ret i8 %bits
}

define i16 @build_byte_pair(i8 %lane0, i8 %lane1) {
  %v0 = insertelement <2 x i8> poison, i8 %lane0, i64 0
  %v1 = insertelement <2 x i8> %v0, i8 %lane1, i64 1
  %bits = bitcast <2 x i8> %v1 to i16
  ret i16 %bits
}

define i32 @build_half_pair(i16 %lane0, i16 %lane1) {
  %v0 = insertelement <2 x i16> poison, i16 %lane0, i64 0
  %v1 = insertelement <2 x i16> %v0, i16 %lane1, i64 1
  %bits = bitcast <2 x i16> %v1 to i32
  ret i32 %bits
}

define i64 @build_octa(i64 %lane0) {
  %vector = insertelement <1 x i64> poison, i64 %lane0, i64 0
  %bits = bitcast <1 x i64> %vector to i64
  ret i64 %bits
}

define i64 @build_double(double %lane0) {
  %vector = insertelement <1 x double> poison, double %lane0, i64 0
  %bits = bitcast <1 x double> %vector to i64
  ret i64 %bits
}

define i64 @constant_wide_mask() {
  %bits = bitcast <64 x i1> zeroinitializer to i64
  ret i64 %bits
}

; CHECK-LABEL: extract_word0:
; CHECK:       SRU r231, r231, 32
define i32 @extract_word0(i64 %bits) {
  %vector = bitcast i64 %bits to <2 x i32>
  %lane = extractelement <2 x i32> %vector, i64 0
  ret i32 %lane
}

; CHECK-LABEL: extract_word1:
; CHECK-NOT:   SRU
; CHECK:       POP 0, 0
define i32 @extract_word1(i64 %bits) {
  %vector = bitcast i64 %bits to <2 x i32>
  %lane = extractelement <2 x i32> %vector, i64 1
  ret i32 %lane
}

; Dynamic lane access scalarizes within one eight-byte temporary and does not
; introduce a runtime helper.
; CHECK-LABEL: extract_byte:
; CHECK-NOT:   PUSHJ
; CHECK:       STBU
; CHECK:       LDBU r231
; CHECK:       POP 0, 0
define i8 @extract_byte(i64 %bits, i64 %index) {
  %vector = bitcast i64 %bits to <8 x i8>
  %lane = extractelement <8 x i8> %vector, i64 %index
  ret i8 %lane
}

; CHECK-LABEL: insert_byte:
; CHECK-NOT:   PUSHJ
; CHECK:       STBU r232
; CHECK:       LDOU r231
; CHECK:       POP 0, 0
define i64 @insert_byte(i64 %bits, i8 %lane, i64 %index) {
  %vector = bitcast i64 %bits to <8 x i8>
  %updated = insertelement <8 x i8> %vector, i8 %lane, i64 %index
  %packed = bitcast <8 x i8> %updated to i64
  ret i64 %packed
}

; Inserting into poison or undef defines only the selected lane. Extracting
; that lane recovers the inserted value without assigning the other lanes a
; target-defined value.
; CHECK-LABEL: poison_lane:
; CHECK-NOT:   PUSHJ
; CHECK:       POP 0, 0
define i32 @poison_lane(i32 %value) {
  %vector = insertelement <2 x i32> poison, i32 %value, i64 0
  %lane = extractelement <2 x i32> %vector, i64 0
  ret i32 %lane
}

; CHECK-LABEL: undef_lane:
; CHECK-NOT:   PUSHJ
; CHECK:       POP 0, 0
define i32 @undef_lane(i32 %value) {
  %vector = insertelement <2 x i32> undef, i32 %value, i64 1
  %lane = extractelement <2 x i32> %vector, i64 1
  ret i32 %lane
}

; An out-of-range constant index remains poison. The target neither traps nor
; calls a helper to impose a value on it.
; CHECK-LABEL: out_of_range_lane:
; CHECK-NOT:   PUSHJ
; CHECK-NOT:   TRAP
; CHECK:       POP 0, 0
define i32 @out_of_range_lane(<2 x i32> %vector) {
  %lane = extractelement <2 x i32> %vector, i64 2
  ret i32 %lane
}
