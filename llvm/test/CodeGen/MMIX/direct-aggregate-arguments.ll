; RUN: llc -mtriple=mmix -verify-machineinstrs -filetype=asm %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=mmix -verify-machineinstrs -stop-after=mmix-isel %s -o - | FileCheck %s --check-prefix=ISEL

target triple = "mmix"

%empty = type {}
%packed5 = type <{ i8, i32 }>
%nested6 = type { i8, { i16, i8 } }
%bytes3 = type [3 x i8]
%float_pair = type { float, i32 }

declare void @take_packed(%packed5)
declare void @take_nested(%nested6)
declare void @take_bytes(%bytes3)
declare void @take_float_pair(%float_pair)
declare void @take_empty(i64, %empty, i64)
declare void @take_last_register(i64, i64, i64, i64, i64, i64, i64, i64,
                                 i64, i64, i64, i64, i64, i64, i64,
                                 %packed5)
declare void @take_first_stack(i64, i64, i64, i64, i64, i64, i64, i64,
                               i64, i64, i64, i64, i64, i64, i64, i64,
                               %packed5)

; A five-byte packed object occupies the low 40 bits. Its leading byte is
; therefore shifted down by 32 bits at the callee.
; ASM-LABEL: packed_leading_byte:
; ASM:       SRU r231, r231, 32
; ASM-NEXT:  POP 0, 0
define i8 @packed_leading_byte(%packed5 %value) nounwind {
  %field = extractvalue %packed5 %value, 0
  ret i8 %field
}

; The trailing i32 is already in the low bits of the direct slot.
; ASM-LABEL: packed_trailing_word:
; ASM-NOT:   SRU
; ASM:       POP 0, 0
define i32 @packed_trailing_word(%packed5 %value) {
  %field = extractvalue %packed5 %value, 1
  ret i32 %field
}

; Caller packing places fields at their object offsets and never sign-extends
; them. Padding has no influence on the represented field values.
; ISEL-LABEL: name: call_packed
; ISEL:       [[TAIL:%[0-9]+]]:{{[^ ]+}} = AND
; ISEL:       [[LEAD:%[0-9]+]]:{{[^ ]+}} = SLUI {{.*}}, 32
; ISEL:       [[PACKED:%[0-9]+]]:{{[^ ]+}} = OR killed [[LEAD]], killed [[TAIL]]
; ISEL:       $r231 = COPY [[PACKED]]
; ISEL:       DIRECT_CALL_STATE @take_packed, {{.*}}implicit $r231
define void @call_packed(i64 %first, i64 %second) {
  %first.byte = trunc i64 %first to i8
  %second.word = trunc i64 %second to i32
  %v0 = insertvalue %packed5 poison, i8 %first.byte, 0
  %v1 = insertvalue %packed5 %v0, i32 %second.word, 1
  call void @take_packed(%packed5 %v1)
  ret void
}

; Nested aggregate offsets include both interior and trailing padding. The
; six-byte object places its nested i16 at bits 16 through 31.
; ISEL-LABEL: name: call_nested
; ISEL:       [[NESTED_WORD:%[0-9]+]]:{{[^ ]+}} = SLUI {{.*}}, 16
; ISEL:       $r231 = COPY {{.*}}[[NESTED_WORD]]
; ISEL:       DIRECT_CALL_STATE @take_nested, {{.*}}implicit $r231
define void @call_nested(i16 %word) {
  %v0 = insertvalue %nested6 zeroinitializer, i16 %word, 1, 0
  call void @take_nested(%nested6 %v0)
  ret void
}

; Arrays use their element offsets in the same right-justified byte sequence.
; ASM-LABEL: array_middle_byte:
; ASM:       SRU r231, r231, 8
; ASM-NEXT:  POP 0, 0
define i8 @array_middle_byte(%bytes3 %value) nounwind {
  %field = extractvalue %bytes3 %value, 1
  ret i8 %field
}

; ISEL-LABEL: name: call_byte_array
; ISEL:       [[MIDDLE:%[0-9]+]]:{{[^ ]+}} = SLUI {{.*}}, 8
; ISEL:       [[HIGH:%[0-9]+]]:{{[^ ]+}} = SLUI {{.*}}, 16
; ISEL:       $r231 = COPY
; ISEL:       DIRECT_CALL_STATE @take_bytes, {{.*}}implicit $r231
define void @call_byte_array(i8 %a, i8 %b, i8 %c) {
  %v0 = insertvalue %bytes3 poison, i8 %a, 0
  %v1 = insertvalue %bytes3 %v0, i8 %b, 1
  %v2 = insertvalue %bytes3 %v1, i8 %c, 2
  call void @take_bytes(%bytes3 %v2)
  ret void
}

; Floating fields retain their raw binary32 representation inside the object.
; ASM-LABEL: float_leading_field:
; ASM:       SRU r231, r231, 32
; ASM-NEXT:  POP 0, 0
define float @float_leading_field(%float_pair %value) nounwind {
  %field = extractvalue %float_pair %value, 0
  ret float %field
}

; ISEL-LABEL: name: call_float_pair
; ISEL:       [[SHORT_FLOAT:%[0-9]+]]:f32bitscodegen = COPY
; ISEL:       [[FLOAT_BITS:%[0-9]+]]:fpr64codegen = COPY killed [[SHORT_FLOAT]]
; ISEL:       [[SHIFTED_FLOAT:%[0-9]+]]:{{[^ ]+}} = SLUI {{.*}}[[FLOAT_BITS]], 32
; ISEL:       $r231 = COPY
; ISEL:       DIRECT_CALL_STATE @take_float_pair, {{.*}}implicit $r231
define void @call_float_pair(float %fp, i32 %integer) {
  %v0 = insertvalue %float_pair poison, float %fp, 0
  %v1 = insertvalue %float_pair %v0, i32 %integer, 1
  call void @take_float_pair(%float_pair %v1)
  ret void
}

; Empty aggregates do not consume a register or stack slot.
; ASM-LABEL: empty_consumes_no_slot:
; ASM:       OR r231, r232, 0
; ASM-NEXT:  POP 0, 0
define i64 @empty_consumes_no_slot(i64 %first, %empty %ignored, i64 %second) nounwind {
  ret i64 %second
}

; Caller allocation also skips the empty value, so the following scalar uses
; r232 rather than r233.
; ISEL-LABEL: name: call_empty_consumes_no_slot
; ISEL:       $r231 = COPY
; ISEL:       $r232 = COPY
; ISEL-NOT:   $r233 = COPY
; ISEL:       DIRECT_CALL_STATE @take_empty, {{.*}}implicit $r231, implicit $r232
define void @call_empty_consumes_no_slot(i64 %first, i64 %second) {
  call void @take_empty(i64 %first, %empty zeroinitializer, i64 %second)
  ret void
}

; The sixteenth ordinary ABI slot is r246.
; ASM-LABEL: aggregate_in_last_register_slot:
; ASM:       OR r231, r246, 0
; ASM-NEXT:  POP 0, 0
define i8 @aggregate_in_last_register_slot(
    i64 %a0, i64 %a1, i64 %a2, i64 %a3, i64 %a4,
    i64 %a5, i64 %a6, i64 %a7, i64 %a8, i64 %a9,
    i64 %a10, i64 %a11, i64 %a12, i64 %a13, i64 %a14,
    %packed5 %value) nounwind {
  %field = extractvalue %packed5 %value, 1
  %byte = trunc i32 %field to i8
  ret i8 %byte
}

; The next aggregate uses one stack octa at incoming offset zero.
; ISEL-LABEL: name: aggregate_in_first_stack_slot
; ISEL:       fixedStack:
; ISEL:       offset: 0, size: 8, alignment: 8
; ISEL:       LDTUI {{.*}} :: (load (s32) from %fixed-stack.0 + 4, basealign 8)
define i32 @aggregate_in_first_stack_slot(
    i64 %a0, i64 %a1, i64 %a2, i64 %a3, i64 %a4, i64 %a5,
    i64 %a6, i64 %a7, i64 %a8, i64 %a9, i64 %a10, i64 %a11,
    i64 %a12, i64 %a13, i64 %a14, i64 %a15, %packed5 %value) {
  %field = extractvalue %packed5 %value, 1
  ret i32 %field
}

; Caller allocation uses the same slot sequence. A direct aggregate after
; fifteen scalar slots is copied to r246.
; ISEL-LABEL: name: call_aggregate_in_last_register_slot
; ISEL:       $r246 = COPY
; ISEL:       DIRECT_CALL_STATE @take_last_register, {{.*}}implicit $r246
define void @call_aggregate_in_last_register_slot(%packed5 %value) {
  call void @take_last_register(
      i64 0, i64 1, i64 2, i64 3, i64 4, i64 5, i64 6, i64 7,
      i64 8, i64 9, i64 10, i64 11, i64 12, i64 13, i64 14,
      %packed5 %value)
  ret void
}

; After sixteen scalar slots, the direct aggregate is stored as one stack
; octa at outgoing offset zero.
; ISEL-LABEL: name: call_aggregate_in_first_stack_slot
; ISEL:       ADJCALLSTACKDOWN 8, 0
; ISEL:       STOUI {{.*}}, 0 :: (store (s64) into stack)
; ISEL:       DIRECT_CALL_STATE @take_first_stack,
define void @call_aggregate_in_first_stack_slot(%packed5 %value) {
  call void @take_first_stack(
      i64 0, i64 1, i64 2, i64 3, i64 4, i64 5, i64 6, i64 7,
      i64 8, i64 9, i64 10, i64 11, i64 12, i64 13, i64 14, i64 15,
      %packed5 %value)
  ret void
}
