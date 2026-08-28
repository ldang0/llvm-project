; RUN: llc -mtriple=mmix -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=mmix -O2 -verify-machineinstrs %s -o - | FileCheck %s

target triple = "mmix"

@words = global <2 x i32> zeroinitializer, align 8

; Fixed-vector memory operations retain their exact object width. In
; particular, scalarization must not turn a narrow object into an eight-byte
; access merely because its call representation occupies one ABI slot.
; CHECK-LABEL: copy_byte:
; CHECK-NOT:   PUSHJ
; CHECK:       LDBU
; CHECK:       STBU
; CHECK:       POP 0, 0
define void @copy_byte(ptr %dst, ptr %src) {
  %value = load <1 x i8>, ptr %src, align 1
  store <1 x i8> %value, ptr %dst, align 1
  ret void
}

; CHECK-LABEL: copy_bytes:
; CHECK-NOT:   PUSHJ
; CHECK:       LDWU
; CHECK:       STWU
; CHECK:       POP 0, 0
define void @copy_bytes(ptr %dst, ptr %src) {
  %value = load <2 x i8>, ptr %src, align 2
  store <2 x i8> %value, ptr %dst, align 2
  ret void
}

; CHECK-LABEL: copy_halves:
; CHECK-NOT:   PUSHJ
; CHECK:       LDTU
; CHECK:       STTU
; CHECK:       POP 0, 0
define void @copy_halves(ptr %dst, ptr %src) {
  %value = load <2 x i16>, ptr %src, align 4
  store <2 x i16> %value, ptr %dst, align 4
  ret void
}

; CHECK-LABEL: copy_words:
; CHECK-NOT:   PUSHJ
; CHECK:       LDOU
; CHECK:       STOU
; CHECK:       POP 0, 0
define void @copy_words(ptr %dst, ptr %src) {
  %value = load <2 x i32>, ptr %src, align 8
  store <2 x i32> %value, ptr %dst, align 8
  ret void
}

; Supported unaligned vector accesses decompose to byte operations and retain
; big-endian object order.
; CHECK-LABEL: copy_unaligned_words:
; CHECK-NOT:   PUSHJ
; CHECK-COUNT-8: LDBU
; CHECK-COUNT-8: STBU
; CHECK:       POP 0, 0
define void @copy_unaligned_words(ptr %dst, ptr %src) {
  %value = load <2 x i32>, ptr %src, align 1
  store <2 x i32> %value, ptr %dst, align 1
  ret void
}

; Volatile accesses remain present and use the vector object's exact width.
; CHECK-LABEL: copy_volatile_words:
; CHECK-NOT:   PUSHJ
; CHECK:       LDTU [[HIGH:r[0-9]+]], r232, 0
; CHECK:       LDTU [[LOW:r[0-9]+]], r232, 4
; CHECK:       STTU [[LOW]], r231, 4
; CHECK:       STTU [[HIGH]], r231, 0
; CHECK:       POP 0, 0
define void @copy_volatile_words(ptr %dst, ptr %src) {
  %value = load volatile <2 x i32>, ptr %src, align 8
  store volatile <2 x i32> %value, ptr %dst, align 8
  ret void
}

; CHECK-LABEL: update_global:
; CHECK-NOT:   PUSHJ
; CHECK-DAG:   LDTU {{r[0-9]+}}, {{r[0-9]+}}, 0
; CHECK-DAG:   LDTU {{r[0-9]+}}, {{r[0-9]+}}, 4
; CHECK-DAG:   STTU {{r[0-9]+}}, {{r[0-9]+}}, 0
; CHECK-DAG:   STTU {{r[0-9]+}}, {{r[0-9]+}}, 4
; CHECK:       POP 0, 0
define void @update_global(<2 x i32> %increment) {
  %old = load <2 x i32>, ptr @words, align 8
  %new = add <2 x i32> %old, %increment
  store <2 x i32> %new, ptr @words, align 8
  ret void
}

; CHECK-LABEL: local_round_trip:
; CHECK-NOT:   PUSHJ
; CHECK:       STOU
; CHECK:       LDOU
; CHECK:       POP 0, 0
define <2 x i32> @local_round_trip(<2 x i32> %value) {
  %slot = alloca <2 x i32>, align 8
  store volatile <2 x i32> %value, ptr %slot, align 8
  %result = load volatile <2 x i32>, ptr %slot, align 8
  ret <2 x i32> %result
}
