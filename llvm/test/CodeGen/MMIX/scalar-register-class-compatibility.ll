; RUN: llc -mtriple=mmix-unknown-linux -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=mmix-unknown-unknown -verify-machineinstrs < %s | FileCheck %s

; Architectural operands and ABI copies share physical registers. Their common
; register class must accept the DAG value type, including in assertion builds.
declare i32 @check(ptr)
declare double @get_double()
declare float @get_float()

define i1 @integer_call_result(ptr %p) {
; CHECK-LABEL: integer_call_result:
; CHECK: PUSHGO
; CHECK: AND
; CHECK: POP
  %value = call i32 @check(ptr %p)
  %zero = icmp eq i32 %value, 0
  ret i1 %zero
}

define double @double_call_result() {
; CHECK-LABEL: double_call_result:
; CHECK: PUSHGO
; CHECK: FADD
; CHECK: POP
  %value = call double @get_double()
  %sum = fadd double %value, 1.0
  ret double %sum
}

define i32 @float_call_result() {
; CHECK-LABEL: float_call_result:
; CHECK: PUSHGO
; CHECK-NOT: FIX
; CHECK: POP
  %value = call float @get_float()
  %bits = bitcast float %value to i32
  ret i32 %bits
}

define i64 @double_bits(double %value) {
; CHECK-LABEL: double_bits:
; CHECK-NOT: FIX
; CHECK: POP
  %bits = bitcast double %value to i64
  ret i64 %bits
}

define double @integer_bits(i64 %value) {
; CHECK-LABEL: integer_bits:
; CHECK-NOT: FLOT
; CHECK: POP
  %bits = bitcast i64 %value to double
  ret double %bits
}
