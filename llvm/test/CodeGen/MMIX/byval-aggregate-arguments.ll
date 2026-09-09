; RUN: llc -mtriple=mmix -verify-machineinstrs -stop-after=mmix-isel %s -o - | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=mmix -verify-machineinstrs -filetype=asm %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=mmix-unknown-elf -verify-machineinstrs -filetype=obj %s -o /dev/null

target triple = "mmix"

%pair = type { i64, i64 }
%triple = type { i32, i32, i32 }
%large = type { [80 x i8] }

declare void @take_pair(ptr byval(%pair) align 8)
declare void @take_triple(ptr byval(%triple) align 4)
declare void @take_large(ptr byval(%large) align 8)
declare void @take_last(i64, i64, i64, i64, i64, i64, i64, i64,
                        i64, i64, i64, i64, i64, i64, i64,
                        ptr byval(%pair) align 8)
declare void @take_stack(i64, i64, i64, i64, i64, i64, i64, i64,
                         i64, i64, i64, i64, i64, i64, i64, i64,
                         ptr byval(%pair) align 8)

; The caller owns a distinct object with the byval type's 16-byte size and
; eight-byte alignment. The frame address, rather than the source pointer, is
; passed in the first ordinary argument slot.
; ISEL-LABEL: name: copy_pair
; ISEL:       stack:
; ISEL-NEXT:    - { id: 0, {{.*}}size: 16, alignment: 8,
; ISEL:       [[SOURCE:%[0-9]+]]:{{[^ ]+}} = COPY $r231
; ISEL:       LDOUI [[SOURCE]], 8
; ISEL:       STOUI {{.*}}%stack.0, 8
; ISEL:       LDOUI [[SOURCE]], 0
; ISEL:       STOUI {{.*}}%stack.0, 0
; ISEL:       [[COPY:%[0-9]+]]:{{[^ ]+}} = ADDUI %stack.0, 0
; ISEL:       $r231 = COPY [[COPY]]
; ISEL:       DIRECT_CALL_STATE @take_pair, {{.*}}implicit $r231
define void @copy_pair(ptr %source) {
  call void @take_pair(ptr byval(%pair) align 8 %source)
  ret void
}

; A non-octa-sized copy retains the exact object size from the byval type.
; ISEL-LABEL: name: copy_triple
; ISEL:       stack:
; ISEL-NEXT:    - { id: 0, {{.*}}size: 12,
; ISEL:       DIRECT_CALL_STATE @take_triple, {{.*}}implicit $r231
define void @copy_triple(ptr %source) {
  call void @take_triple(ptr byval(%triple) align 4 %source)
  ret void
}

; Copies beyond the inline threshold use the ordinary memcpy ABI. Its call
; sequence finishes before the sequence for the requested call begins.
; ISEL-LABEL: name: copy_large
; ISEL:       stack:
; ISEL-NEXT:    - { id: 0, {{.*}}size: 80, alignment: 8,
; ISEL:       ADJCALLSTACKDOWN 0, 0
; ISEL:       DIRECT_CALL_STATE &memcpy, {{.*}}implicit $r231, implicit $r232, implicit $r233
; ISEL:       ADJCALLSTACKUP 0, 0
; ISEL:       ADJCALLSTACKDOWN 0, 0
; ISEL:       DIRECT_CALL_STATE @take_large, {{.*}}implicit $r231
; ISEL:       ADJCALLSTACKUP 0, 0
define void @copy_large(ptr %source) {
  call void @take_large(ptr byval(%large) align 8 %source)
  ret void
}

; A byval pointer consumes one ordinary slot. After fifteen scalar slots it
; occupies the last argument register.
; ISEL-LABEL: name: copy_in_last_register_slot
; ISEL:       $r246 = COPY
; ISEL:       DIRECT_CALL_STATE @take_last, {{.*}}implicit $r246
define void @copy_in_last_register_slot(ptr %source) {
  call void @take_last(
      i64 0, i64 1, i64 2, i64 3, i64 4, i64 5, i64 6, i64 7,
      i64 8, i64 9, i64 10, i64 11, i64 12, i64 13, i64 14,
      ptr byval(%pair) align 8 %source)
  ret void
}

; After sixteen scalar slots the pointer is stored in the first outgoing
; stack slot; the pointee bytes do not consume argument slots.
; ISEL-LABEL: name: copy_in_first_stack_slot
; ISEL:       ADJCALLSTACKDOWN 8, 0
; ISEL:       STOUI {{.*}}, 0 :: (store (s64) into stack)
; ISEL:       DIRECT_CALL_STATE @take_stack
define void @copy_in_first_stack_slot(ptr %source) {
  call void @take_stack(
      i64 0, i64 1, i64 2, i64 3, i64 4, i64 5, i64 6, i64 7,
      i64 8, i64 9, i64 10, i64 11, i64 12, i64 13, i64 14, i64 15,
      ptr byval(%pair) align 8 %source)
  ret void
}

; The callee receives the source address in the ordinary register slot and
; initializes its own by-value object before exposing the parameter.
; ISEL-LABEL: name: read_register_copy
; ISEL:       stack:
; ISEL-NEXT:    - { id: 0, {{.*}}size: 16, alignment: 8,
; ISEL:       [[ADDRESS:%[0-9]+]]:{{[^ ]+}} = COPY $r231
; ISEL:       LDOUI [[ADDRESS]], 0
; ISEL:       STOUI {{.*}}%stack.0, 0
; ISEL:       LDOUI [[ADDRESS]], 8
; ISEL:       STOUI {{.*}}%stack.0, 8
define i64 @read_register_copy(ptr byval(%pair) align 8 %value) {
  %field = getelementptr %pair, ptr %value, i64 0, i32 1
  %loaded = load i64, ptr %field, align 8
  ret i64 %loaded
}

; A source address received in the seventeenth ordinary slot also initializes
; an independent local object.
; ISEL-LABEL: name: read_stack_copy
; ISEL:       fixedStack:
; ISEL-NEXT:    - { id: 0, {{.*}}offset: 0, size: 8, alignment: 8,
; ISEL:       stack:
; ISEL-NEXT:    - { id: 0, {{.*}}size: 16, alignment: 8,
; ISEL:       [[ADDRESS:%[0-9]+]]:{{[^ ]+}} = LDOUI %fixed-stack.0, 0
; ISEL:       LDOUI [[ADDRESS]], 0
; ISEL:       STOUI {{.*}}%stack.0, 0
; ISEL:       LDOUI [[ADDRESS]], 8
; ISEL:       STOUI {{.*}}%stack.0, 8
define i64 @read_stack_copy(
    i64 %a0, i64 %a1, i64 %a2, i64 %a3, i64 %a4, i64 %a5,
    i64 %a6, i64 %a7, i64 %a8, i64 %a9, i64 %a10, i64 %a11,
    i64 %a12, i64 %a13, i64 %a14, i64 %a15,
    ptr byval(%pair) align 8 %value) {
  %field = getelementptr %pair, ptr %value, i64 0, i32 1
  %loaded = load i64, ptr %field, align 8
  ret i64 %loaded
}

; Writes through a byval parameter target the callee-owned object, not the
; source address received from the caller.
; ISEL-LABEL: name: mutate_register_copy
; ISEL:       stack:
; ISEL-NEXT:    - { id: 0, {{.*}}size: 16, alignment: 8,
; ISEL:       COPY $r231
; ISEL:       STOUI {{.*}}%stack.0, 0
; ISEL:       STOUI {{.*}}%stack.0, 8
; ISEL:       STOUI {{.*}}%stack.0, 0
define void @mutate_register_copy(ptr byval(%pair) align 8 %value) {
  %field = getelementptr %pair, ptr %value, i64 0, i32 0
  store i64 99, ptr %field, align 8
  ret void
}

; Forwarding a byval parameter creates another distinct caller-owned copy.
; ISEL-LABEL: name: forward_copy
; ISEL:       [[SOURCE:%[0-9]+]]:{{[^ ]+}} = COPY $r231
; ISEL:       LDOUI [[SOURCE]], 8
; ISEL:       STOUI {{.*}}%stack.0, 8
; ISEL:       LDOUI [[SOURCE]], 0
; ISEL:       STOUI {{.*}}%stack.0, 0
; ISEL:       [[COPY:%[0-9]+]]:{{[^ ]+}} = ADDUI %stack.0, 0
; ISEL:       $r231 = COPY [[COPY]]
define void @forward_copy(ptr byval(%pair) align 8 %value) {
  call void @take_pair(ptr byval(%pair) align 8 %value)
  ret void
}

; Fixed-frame-pointer functions address the caller-owned copy relative to
; r253 and keep it separate from the saved frame pointer.
; ASM-LABEL: copy_with_frame_pointer:
; ASM:       STOU r253, r254, 16
; ASM:       ADDU r253, r254, 24
; ASM:       STOU {{r[0-9]+}}, r253, r255
; ASM:       STOU {{r[0-9]+}}, r253, r255
; ASM:       ADDU r231, r253, r255
; ASM:       PUSHGO
define void @copy_with_frame_pointer(ptr %source) nounwind #0 {
  call void @take_pair(ptr byval(%pair) align 8 %source)
  ret void
}

attributes #0 = { "frame-pointer"="all" }
