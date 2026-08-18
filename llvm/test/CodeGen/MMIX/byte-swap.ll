; RUN: llc -mtriple=mmix -O0 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=mmix -O1 -verify-machineinstrs %s -o /dev/null
; RUN: llc -mtriple=mmix -O2 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=mmix -O3 -verify-machineinstrs %s -o /dev/null

target triple = "mmix"

declare i64 @llvm.bswap.i64(i64)

define i64 @byte_swap_i64(i64 %value) {
; CHECK-LABEL: byte_swap_i64:
; CHECK-NOT:   __bswap
; CHECK-DAG:   SLU
; CHECK-DAG:   SRU
; CHECK-DAG:   AND
; CHECK-DAG:   OR
; CHECK-NOT:   __bswap
; CHECK:       POP 0, 0
  %result = call i64 @llvm.bswap.i64(i64 %value)
  ret i64 %result
}
