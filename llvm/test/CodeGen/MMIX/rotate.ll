; RUN: llc -mtriple=mmix -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=mmix -O1 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=mmix -O2 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=mmix -O3 -verify-machineinstrs %s -o /dev/null

target triple = "mmix"

declare i64 @llvm.fshl.i64(i64, i64, i64)
declare i64 @llvm.fshr.i64(i64, i64, i64)

define i64 @rotate_left_variable(i64 %value, i64 %amount) {
; CHECK-LABEL: rotate_left_variable:
; CHECK-DAG:   AND
; CHECK-DAG:   SLU
; CHECK-DAG:   SRU
; CHECK:       OR
; CHECK:       POP 0, 0
  %result = call i64 @llvm.fshl.i64(i64 %value, i64 %value, i64 %amount)
  ret i64 %result
}

define i64 @rotate_right_variable(i64 %value, i64 %amount) {
; CHECK-LABEL: rotate_right_variable:
; CHECK-DAG:   AND
; CHECK-DAG:   SRU
; CHECK-DAG:   SLU
; CHECK:       OR
; CHECK:       POP 0, 0
  %result = call i64 @llvm.fshr.i64(i64 %value, i64 %value, i64 %amount)
  ret i64 %result
}

define i64 @rotate_left_zero(i64 %value) {
; CHECK-LABEL: rotate_left_zero:
; CHECK-NOT:   SLU
; CHECK-NOT:   SRU
; CHECK:       POP 0, 0
  %result = call i64 @llvm.fshl.i64(i64 %value, i64 %value, i64 0)
  ret i64 %result
}

define i64 @rotate_right_63(i64 %value) {
; CHECK-LABEL: rotate_right_63:
; CHECK-DAG:   SRU
; CHECK-DAG:   SLU
; CHECK:       OR
; CHECK:       POP 0, 0
  %result = call i64 @llvm.fshr.i64(i64 %value, i64 %value, i64 63)
  ret i64 %result
}

define i64 @rotate_left_64(i64 %value) {
; CHECK-LABEL: rotate_left_64:
; CHECK-NOT:   SLU
; CHECK-NOT:   SRU
; CHECK:       POP 0, 0
  %result = call i64 @llvm.fshl.i64(i64 %value, i64 %value, i64 64)
  ret i64 %result
}
