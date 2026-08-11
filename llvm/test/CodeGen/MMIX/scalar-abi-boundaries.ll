; RUN: llc -mtriple=mmix -verify-machineinstrs -stop-after=mmix-isel %s -o - | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=mmix -verify-machineinstrs -filetype=asm %s -o - | FileCheck %s --check-prefix=ASM

target triple = "mmix"

declare zeroext i1 @boolean_callee(i1 zeroext)
declare ptr @pointer_callee(ptr)
declare void @stack_scalar_callee(
    i64, i64, i64, i64, i64, i64, i64, i64,
    i64, i64, i64, i64, i64, i64, i64, i64,
    i8 signext, i32 zeroext, float)

; Boolean arguments are zero-extended into the complete first octa, and a
; boolean result defines only bit zero of the value copied from $231.
; ISEL-LABEL: name: call_boolean
; ISEL:       [[BOOL:%[0-9]+]]:{{[^ ]+}} = ANDI {{.*}}, 1
; ISEL:       $r231 = COPY [[BOOL]]
; ISEL:       DIRECT_CALL_STATE @boolean_callee, {{.*}}csr_mmix{{.*}}implicit $r231
; ISEL:       [[BOOL_RESULT:%[0-9]+]]:{{[^ ]+}} = COPY $r231
define zeroext i1 @call_boolean(i64 %value) {
  %narrow = trunc i64 %value to i1
  %result = call zeroext i1 @boolean_callee(i1 zeroext %narrow)
  ret i1 %result
}

; Object pointers use the same complete-octa argument and result location.
; ISEL-LABEL: name: call_pointer
; ISEL:       $r231 = COPY %{{[0-9]+}}
; ISEL:       DIRECT_CALL_STATE @pointer_callee, {{.*}}csr_mmix{{.*}}implicit $r231
; ISEL:       %{{[0-9]+}}:gpr64codegen = COPY $r231
define ptr @call_pointer(ptr %value) {
  %result = call ptr @pointer_callee(ptr %value)
  ret ptr %result
}

; Slots 0 through 15 consume $231 through $246. The signed byte, unsigned
; word, and raw binary32 value then occupy complete octas at sp+0, sp+8, and
; sp+16. The f32 bits are not numerically promoted to a double call operand.
; ISEL-LABEL: name: call_stack_scalars
; ISEL:       ADJCALLSTACKDOWN 24, 0
; ISEL-DAG:   STOUI {{.*}}, 0 :: (store (s64) into stack)
; ISEL-DAG:   STOUI {{.*}}, 8 :: (store (s64) into stack + 8)
; ISEL-DAG:   STOUI {{.*}}, 16 :: (store (s64) into stack + 16)
; ISEL:       DIRECT_CALL_STATE @stack_scalar_callee, {{.*}}csr_mmix{{.*}}implicit $r231, implicit $r232, implicit $r233, implicit $r234, implicit $r235, implicit $r236, implicit $r237, implicit $r238, implicit $r239, implicit $r240, implicit $r241, implicit $r242, implicit $r243, implicit $r244, implicit $r245, implicit $r246
; ISEL-NEXT:  ADJCALLSTACKUP 24, 0
; ASM-LABEL: call_stack_scalars:
; ASM:       SUBU r254, r254,
; ASM:       OR [[CALL_SP:r[0-9]+]], r254, 0
; ASM-DAG:   STOU {{r[0-9]+}}, [[CALL_SP]], 0
; ASM-DAG:   STOU {{r[0-9]+}}, [[CALL_SP]], 8
; ASM-DAG:   STOU {{r[0-9]+}}, [[CALL_SP]], 16
define void @call_stack_scalars(i64 %signed, i64 %unsigned, float %short) {
  %signed.narrow = trunc i64 %signed to i8
  %unsigned.narrow = trunc i64 %unsigned to i32
  call void @stack_scalar_callee(
      i64 0, i64 1, i64 2, i64 3, i64 4, i64 5, i64 6, i64 7,
      i64 8, i64 9, i64 10, i64 11, i64 12, i64 13, i64 14, i64 15,
      i8 signext %signed.narrow, i32 zeroext %unsigned.narrow, float %short)
  ret void
}

; The first stack slot contains the already sign-extended complete octa. The
; sext after formal-argument truncation can therefore recover that value.
; ISEL-LABEL: name: formal_stack_signext
; ISEL:       fixedStack:
; ISEL:       offset: 0, size: 8, alignment: 8
; ISEL:       [[SIGNED:%[0-9]+]]:{{[^ ]+}} = LDOUI %fixed-stack.0, 0
; ISEL:       $r231 = COPY [[SIGNED]]
define i64 @formal_stack_signext(
    i64 %a0, i64 %a1, i64 %a2, i64 %a3,
    i64 %a4, i64 %a5, i64 %a6, i64 %a7,
    i64 %a8, i64 %a9, i64 %a10, i64 %a11,
    i64 %a12, i64 %a13, i64 %a14, i64 %a15,
    i8 signext %value) {
  %wide = sext i8 %value to i64
  ret i64 %wide
}

; The unsigned word is slot 17 and begins at sp+8.
; ISEL-LABEL: name: formal_stack_zeroext
; ISEL:       fixedStack:
; ISEL:       offset: 8, size: 8, alignment: 8
; ISEL:       [[UNSIGNED:%[0-9]+]]:{{[^ ]+}} = LDOUI %fixed-stack.{{[0-9]+}}, 0
; ISEL:       $r231 = COPY [[UNSIGNED]]
define i64 @formal_stack_zeroext(
    i64 %a0, i64 %a1, i64 %a2, i64 %a3,
    i64 %a4, i64 %a5, i64 %a6, i64 %a7,
    i64 %a8, i64 %a9, i64 %a10, i64 %a11,
    i64 %a12, i64 %a13, i64 %a14, i64 %a15,
    i8 signext %skip, i32 zeroext %value) {
  %wide = zext i32 %value to i64
  ret i64 %wide
}

; The binary32 stack value is slot 18 at sp+16. Loading the complete octa and
; returning its low tetra preserves the big-endian [sp+20,sp+24) value bytes.
; ISEL-LABEL: name: formal_stack_f32
; ISEL:       fixedStack:
; ISEL:       offset: 16, size: 8, alignment: 8
; ISEL:       [[SHORT:%[0-9]+]]:{{[^ ]+}} = LDOUI %fixed-stack.{{[0-9]+}}, 0
; ISEL:       $r231 = COPY [[SHORT]]
define float @formal_stack_f32(
    i64 %a0, i64 %a1, i64 %a2, i64 %a3,
    i64 %a4, i64 %a5, i64 %a6, i64 %a7,
    i64 %a8, i64 %a9, i64 %a10, i64 %a11,
    i64 %a12, i64 %a13, i64 %a14, i64 %a15,
    i8 signext %skip0, i32 zeroext %skip1, float %value) {
  ret float %value
}
