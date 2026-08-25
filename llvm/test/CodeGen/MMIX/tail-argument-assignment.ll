; RUN: llc -mtriple=mmix -stop-after=finalize-isel -verify-machineinstrs %s -o - \
; RUN:   | FileCheck %s

; Every incoming value is captured in a virtual register before any
; destination register is overwritten by an eligible C tail transfer.

declare i64 @zero_args()
declare i64 @three_args(i64, i64, i64)
declare i64 @sixteen_args(i64, i64, i64, i64, i64, i64, i64, i64,
                          i64, i64, i64, i64, i64, i64, i64, i64)
declare i64 @converted_args(i8 signext, i16 zeroext, float, double)

define i64 @zero_slots() {
; CHECK-LABEL: name: zero_slots
; CHECK-NOT: ADJCALLSTACK
; CHECK-NOT: $r23{{[1-9]}} = COPY
; CHECK: MATERIALIZED_DIRECT_TAIL_STATE @zero_args
  %result = tail call i64 @zero_args()
  ret i64 %result
}

define i64 @permutation_and_repeat(i64 %a, i64 %b, i64 %c) {
; CHECK-LABEL: name: permutation_and_repeat
; CHECK-DAG: [[A:%[0-9]+]]:gpr64codegen = COPY $r231
; CHECK-DAG: [[B:%[0-9]+]]:gpr64codegen = COPY $r232
; CHECK: $r231 = COPY [[B]]
; CHECK-NEXT: $r232 = COPY [[A]]
; CHECK-NEXT: $r233 = COPY [[B]]
; CHECK: MATERIALIZED_DIRECT_TAIL_STATE @three_args
  %result = tail call i64 @three_args(i64 %b, i64 %a, i64 %b)
  ret i64 %result
}

define i64 @sixteen_slot_cycles(
    i64 %a0, i64 %a1, i64 %a2, i64 %a3,
    i64 %a4, i64 %a5, i64 %a6, i64 %a7,
    i64 %a8, i64 %a9, i64 %a10, i64 %a11,
    i64 %a12, i64 %a13, i64 %a14, i64 %a15) {
; CHECK-LABEL: name: sixteen_slot_cycles
; CHECK-DAG: [[A0:%[0-9]+]]:gpr64codegen = COPY $r231
; CHECK-DAG: [[A1:%[0-9]+]]:gpr64codegen = COPY $r232
; CHECK-DAG: [[A14:%[0-9]+]]:gpr64codegen = COPY $r245
; CHECK-DAG: [[A15:%[0-9]+]]:gpr64codegen = COPY $r246
; CHECK: $r231 = COPY [[A1]]
; CHECK-NEXT: $r232 = COPY [[A0]]
; CHECK: $r245 = COPY [[A15]]
; CHECK-NEXT: $r246 = COPY [[A14]]
; CHECK: MATERIALIZED_DIRECT_TAIL_STATE @sixteen_args
  %result = tail call i64 @sixteen_args(
      i64 %a1, i64 %a0, i64 %a2, i64 %a3,
      i64 %a4, i64 %a5, i64 %a6, i64 %a7,
      i64 %a8, i64 %a9, i64 %a10, i64 %a11,
      i64 %a12, i64 %a13, i64 %a15, i64 %a14)
  ret i64 %result
}

define i64 @narrow_and_floating(i8 %s, i16 %z, float %f, double %d) {
; CHECK-LABEL: name: narrow_and_floating
; CHECK: $r231 = COPY
; CHECK-NEXT: $r232 = COPY
; CHECK-NEXT: $r233 = COPY
; CHECK-NEXT: $r234 = COPY
; CHECK: MATERIALIZED_DIRECT_TAIL_STATE @converted_args
  %result = tail call i64 @converted_args(i8 signext %s, i16 zeroext %z,
                                           float %f, double %d)
  ret i64 %result
}

define i64 @indirect_callee_overlap(ptr %callee, i64 %value) {
; CHECK-LABEL: name: indirect_callee_overlap
; CHECK: [[CALLEE:%[0-9]+]]:{{[^ ]+}} = COPY $r231
; CHECK: $r231 = COPY
; CHECK: $r232 = COPY [[CALLEE]]
; CHECK: INDIRECT_TAIL_STATE [[CALLEE]]
  %result = tail call i64 %callee(i64 %value, ptr %callee)
  ret i64 %result
}
