; RUN: llc -mtriple=mmix -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=mmix -O1 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=mmix -O2 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=mmix -O3 -verify-machineinstrs %s -o /dev/null

target triple = "mmix"

@flag = internal global i1 false, align 4

define i64 @load_boolean_zero_extended() {
; CHECK-LABEL: load_boolean_zero_extended:
; CHECK:       LDBU
; CHECK:       POP 0, 0
  %value = load i1, ptr @flag, align 4
  %extended = zext i1 %value to i64
  ret i64 %extended
}

define i64 @load_boolean_sign_extended() {
; CHECK-LABEL: load_boolean_sign_extended:
; CHECK:       LDBU
; CHECK:       SL
; CHECK:       SR
; CHECK:       POP 0, 0
  %value = load i1, ptr @flag, align 4
  %extended = sext i1 %value to i64
  ret i64 %extended
}

define void @store_boolean(i1 %value) {
; CHECK-LABEL: store_boolean:
; CHECK:       STBU
; CHECK:       POP 0, 0
  store i1 %value, ptr @flag, align 4
  ret void
}
