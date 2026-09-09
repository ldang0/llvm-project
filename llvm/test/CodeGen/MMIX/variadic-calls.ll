; RUN: llc -mtriple=mmix-unknown-elf -verify-machineinstrs \
; RUN:   -stop-after=mmix-isel %s -o - | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=mmix-unknown-elf -verify-machineinstrs -filetype=asm \
; RUN:   %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=mmix-unknown-elf -verify-machineinstrs -filetype=obj \
; RUN:   %s -o /dev/null

target triple = "mmix-unknown-elf"

%small = type <{ i8, i32 }>
%large = type { i64, i64 }
%empty = type {}

declare i64 @variadic(i64, ...)
declare void @no_fixed(...)
declare void @with_empty(i64, ...)
declare void @variadic_sret(ptr sret(%large) align 8, i64, ...)
declare void @many(i64, i64, i64, i64, i64, i64, i64, i64,
                   i64, i64, i64, i64, i64, i64, i64, ...)

; Named and unnamed values share one slot sequence. The caller supplies the
; C promotion attributes on i32 operands, passes double without a separate FP
; bank, packs the direct aggregate, and passes one caller-copy address.
; The only local stack object is the 16-byte byval copy; there is no caller
; register-save area and no outgoing stack argument.
; ISEL-LABEL: name: call_mixed
; ISEL:       stack:
; ISEL-NEXT:    - { id: 0, {{.*}}size: 16, alignment: 8,
; ISEL:       [[UNSIGNED:%[0-9]+]]:{{[^ ]+}} = AND
; ISEL:       ADJCALLSTACKDOWN 0, 0
; ISEL:       [[SIGNED:%[0-9]+]]:{{[^ ]+}} = SRI {{.*}}, 32
; ISEL:       $r231 = COPY
; ISEL:       $r232 = COPY [[SIGNED]]
; ISEL:       $r233 = COPY [[UNSIGNED]]
; ISEL:       $r234 = COPY
; ISEL:       $r235 = COPY
; ISEL:       $r236 = COPY
; ISEL:       $r237 = COPY
; ISEL:       DIRECT_CALL_STATE @variadic{{.*}}implicit $r231, implicit $r232, implicit $r233, implicit $r234, implicit $r235, implicit $r236, implicit $r237{{.*}}implicit-def $r231
define i64 @call_mixed(i32 %signed, i32 %unsigned, double %fp, ptr %pointer,
                       %small %small, ptr %large) {
  %result = call i64 (i64, ...) @variadic(
      i64 0, i32 signext %signed, i32 zeroext %unsigned, double %fp,
      ptr %pointer, %small %small, ptr byval(%large) align 8 %large)
  ret i64 %result
}

; An unprototyped call has no named slots, so promoted values begin at $231.
; ISEL-LABEL: name: call_no_fixed
; ISEL:       $r231 = COPY
; ISEL:       $r232 = COPY
; ISEL:       DIRECT_CALL_STATE @no_fixed{{.*}}implicit $r231, implicit $r232
define void @call_no_fixed(i32 %value) {
  call void (...) @no_fixed(i32 signext %value, double 1.0)
  ret void
}

; A frontend-coerced narrow direct aggregate uses noext to distinguish its
; unextended object bits from a promoted C scalar.
; ISEL-LABEL: name: call_coerced_direct
; ISEL:       $r231 = COPY
; ISEL:       $r232 = COPY
; ISEL:       DIRECT_CALL_STATE @no_fixed{{.*}}implicit $r231, implicit $r232
define void @call_coerced_direct(i32 %value) {
  call void (...) @no_fixed(i8 noext 1, i32 noext %value)
  ret void
}

; Empty unnamed aggregates consume no slot. The promoted i32 following the
; empty value therefore uses $232 immediately after the fixed $231 slot.
; ISEL-LABEL: name: call_empty
; ISEL:       $r231 = COPY
; ISEL:       $r232 = COPY
; ISEL-NOT:   $r233 = COPY
; ISEL:       DIRECT_CALL_STATE @with_empty{{.*}}implicit $r231, implicit $r232
define void @call_empty(i32 %value) {
  call void (i64, ...) @with_empty(
      i64 0, %empty zeroinitializer, i32 zeroext %value)
  ret void
}

; The hidden result pointer remains outside the ordinary variadic sequence.
; The named i64 and first unnamed i32 still use $231 and $232.
; ISEL-LABEL: name: call_sret
; ISEL:       $r251 = COPY
; ISEL:       $r231 = COPY
; ISEL:       $r232 = COPY
; ISEL:       DIRECT_CALL_STATE @variadic_sret{{.*}}implicit $r251, implicit $r231, implicit $r232
define void @call_sret(ptr %out, i32 %value) {
  call void (ptr, i64, ...) @variadic_sret(
      ptr sret(%large) align 8 %out, i64 0, i32 signext %value)
  ret void
}

; Fifteen named slots leave $246 for the first unnamed value. Every later
; unnamed value occupies one consecutive stack octa, regardless of class.
; ISEL-LABEL: name: call_stack
; ISEL:       stack:
; ISEL-NEXT:    - { id: 0, {{.*}}size: 16, alignment: 8,
; ISEL:       ADJCALLSTACKDOWN 32, 0
; ISEL-DAG:   STOUI {{.*}}, {{%[0-9]+}}, 0 :: (store (s64) into stack)
; ISEL-DAG:   STOUI {{.*}}, {{%[0-9]+}}, 8 :: (store (s64) into stack + 8)
; ISEL-DAG:   STOUI {{.*}}, {{%[0-9]+}}, 16 :: (store (s64) into stack + 16)
; ISEL-DAG:   STOUI {{.*}}, {{%[0-9]+}}, 24 :: (store (s64) into stack + 24)
; ISEL:       $r231 = COPY
; ISEL:       $r245 = COPY
; ISEL:       $r246 = COPY
; ISEL:       DIRECT_CALL_STATE @many{{.*}}implicit $r231{{.*}}implicit $r246
; ASM-LABEL: call_stack:
; ASM:       SUBU r254, r254, 48
; ASM-DAG:   STOU {{r[0-9]+}}, {{r[0-9]+}}, 0
; ASM-DAG:   STOU {{r[0-9]+}}, {{r[0-9]+}}, 8
; ASM-DAG:   STOU {{r[0-9]+}}, {{r[0-9]+}}, 16
; ASM-DAG:   STOU {{r[0-9]+}}, {{r[0-9]+}}, 24
define void @call_stack(%small %small, ptr %large, ptr %pointer, double %fp,
                        i32 %integer) nounwind {
  call void (i64, i64, i64, i64, i64, i64, i64, i64,
             i64, i64, i64, i64, i64, i64, i64, ...) @many(
      i64 0, i64 1, i64 2, i64 3, i64 4, i64 5, i64 6, i64 7,
      i64 8, i64 9, i64 10, i64 11, i64 12, i64 13, i64 14,
      i32 zeroext %integer, double %fp, ptr %pointer, %small %small,
      ptr byval(%large) align 8 %large)
  ret void
}

; Function-pointer calls use the same named/unnamed allocation.
; ISEL-LABEL: name: call_indirect
; ISEL:       $r231 = COPY
; ISEL:       $r232 = COPY
; ISEL:       $r233 = COPY
; ISEL:       CALL_STATE {{%[0-9]+}}{{.*}}implicit $r231, implicit $r232, implicit $r233
; ASM-LABEL: call_indirect:
; ASM:       PUSHGO r31, {{r[0-9]+}}, 0
define void @call_indirect(ptr %callee, i32 %value) {
  call void (i64, ...) %callee(i64 0, i32 signext %value, double 2.0)
  ret void
}
