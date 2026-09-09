; RUN: llc -mtriple=mmix -verify-machineinstrs -filetype=asm %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=mmix -stop-after=mmix-isel %s -o - | FileCheck %s --check-prefix=ISEL

target triple = "mmix"

; ASM-LABEL: return_void:
; ASM:       POP 0, 0
; ISEL-LABEL: name: return_void
; ISEL:       RET implicit $rj, implicit $rl, implicit $ro, implicit $rg
define void @return_void() {
  ret void
}

; ASM-LABEL: return_i64:
; ASM:       POP 0, 0
; ISEL-LABEL: name: return_i64
; ISEL:       liveins: $r231
; ISEL:       %{{[0-9]+}}:gpr64codegen = COPY $r231
; ISEL:       RET_VALUE {{.*}}implicit $r231
define i64 @return_i64(i64 %value) {
  ret i64 %value
}

; ASM-LABEL: return_f64:
; ASM:       POP 0, 0
; ISEL-LABEL: name: return_f64
; ISEL:       liveins: $r231
; ISEL:       %{{[0-9]+}}:fpr64codegen = COPY $r231
define double @return_f64(double %value) {
  ret double %value
}

; ASM-LABEL: return_signext_i8:
; ASM:       POP 0, 0
define signext i8 @return_signext_i8(i8 signext %value) {
  ret i8 %value
}

; ASM-LABEL: return_zeroext_i32:
; ASM:       POP 0, 0
define zeroext i32 @return_zeroext_i32(i32 zeroext %value) {
  ret i32 %value
}

; ASM-LABEL: return_f32:
; ASM:       POP 0, 0
define float @return_f32(float %value) {
  ret float %value
}

; ASM-LABEL: return_pointer:
; ASM:       POP 0, 0
define ptr @return_pointer(ptr %value) {
  ret ptr %value
}

; ASM-LABEL: return_last_reg_arg:
; ASM:       OR r231, r246, 0
; ASM-NEXT:  POP 0, 0
; ISEL-LABEL: name: return_last_reg_arg
; ISEL:       liveins: $r246
define i64 @return_last_reg_arg(
    i64 %a0, i64 %a1, i64 %a2, i64 %a3,
    i64 %a4, i64 %a5, i64 %a6, i64 %a7,
    i64 %a8, i64 %a9, i64 %a10, i64 %a11,
    i64 %a12, i64 %a13, i64 %a14, i64 %a15) nounwind {
  ret i64 %a15
}

; ASM-LABEL: return_stack_arg:
; ASM:       LDOU r231, r254, 0
; ASM-NEXT:  POP 0, 0
; ISEL-LABEL: name: return_stack_arg
; ISEL:       fixedStack:
; ISEL:       offset: 0, size: 8, alignment: 8
; ISEL:       LDOUI %fixed-stack.0, 0
define i64 @return_stack_arg(
    i64 %a0, i64 %a1, i64 %a2, i64 %a3,
    i64 %a4, i64 %a5, i64 %a6, i64 %a7,
    i64 %a8, i64 %a9, i64 %a10, i64 %a11,
    i64 %a12, i64 %a13, i64 %a14, i64 %a15,
    i64 %a16) nounwind {
  ret i64 %a16
}

; ASM-LABEL: return_second_stack_arg:
; ASM:       LDOU r231, r254, 8
; ASM-NEXT:  POP 0, 0
; ISEL-LABEL: name: return_second_stack_arg
; ISEL:       offset: 8, size: 8, alignment: 8
; ISEL:       LDOUI %fixed-stack.{{[0-9]+}}, 0
define double @return_second_stack_arg(
    i64 %a0, i64 %a1, i64 %a2, i64 %a3,
    i64 %a4, i64 %a5, i64 %a6, i64 %a7,
    i64 %a8, i64 %a9, i64 %a10, i64 %a11,
    i64 %a12, i64 %a13, i64 %a14, i64 %a15,
    double %a16, double %a17) nounwind {
  ret double %a17
}
