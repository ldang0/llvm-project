; RUN: llc -mtriple=mmix -verify-machineinstrs -stop-after=mmix-isel %s -o - | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=mmix -verify-machineinstrs -filetype=asm %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=mmix-unknown-elf -verify-machineinstrs -filetype=obj %s -o /dev/null

target triple = "mmix"

%pair = type { i64, i64 }

declare void @external_pair(ptr sret(%pair) align 8, i64)
declare void @helper()

; The hidden result pointer arrives in $251 without consuming the first
; ordinary argument slot. Every return copies the retained address to $231.
; ISEL-LABEL: name: store_pair
; ISEL:       liveins: $r251, $r231, $r232
; ISEL:       [[SRET:%[0-9]+]]:{{[^ ]+}} = COPY $r251
; ISEL:       $r231 = COPY [[SRET]]
; ISEL-NEXT:  RET_VALUE {{.*}}implicit $r231
; ISEL:       $r231 = COPY [[SRET]]
; ISEL-NEXT:  RET_VALUE {{.*}}implicit $r231
define internal void @store_pair(ptr sret(%pair) align 8 %out, i64 %value,
                                 i1 %select_second) {
entry:
  br i1 %select_second, label %second, label %first

first:
  %first_ptr = getelementptr inbounds %pair, ptr %out, i64 0, i32 0
  store i64 %value, ptr %first_ptr, align 8
  ret void

second:
  %second_ptr = getelementptr inbounds %pair, ptr %out, i64 0, i32 1
  store i64 %value, ptr %second_ptr, align 8
  ret void
}

; $251 is caller-clobbered. The incoming pointer is retained in a local
; register across a nested call and remains available for both the store and
; the required address result.
; ASM-LABEL: nested_result:
; ASM:       OR [[SRET:r[0-9]+]], r251, 0
; ASM:       PUSHGO
; ASM:       STOU {{r[0-9]+}}, [[SRET]], 0
; ASM:       OR r231, [[SRET]], 0
; ASM:       POP 0, 0
define void @nested_result(ptr sret(%pair) align 8 %out, i64 %value) {
entry:
  call void @helper()
  %first_ptr = getelementptr inbounds %pair, ptr %out, i64 0, i32 0
  store i64 %value, ptr %first_ptr, align 8
  ret void
}

; A local direct call places the hidden pointer in $251 while the first user
; argument remains in $231.
; ISEL-LABEL: name: call_local
; ISEL:       $r251 = COPY [[OUT:%[0-9]+]]
; ISEL:       $r231 = COPY [[VALUE:%[0-9]+]]
; ISEL:       $r232 = COPY
; ISEL:       CALL_STATE @store_pair{{.*}}implicit $r251, implicit $r231, implicit $r232
define void @call_local(ptr %out, i64 %value) {
entry:
  call void @store_pair(ptr sret(%pair) align 8 %out, i64 %value,
                        i1 false)
  ret void
}

; External calls use the same register assignment and model $251 as a call
; input under the caller-clobbered MMIX register mask.
; ISEL-LABEL: name: call_external
; ISEL:       $r251 = COPY [[OUT:%[0-9]+]]
; ISEL:       $r231 = COPY [[VALUE:%[0-9]+]]
; ISEL:       DIRECT_CALL_STATE @external_pair{{.*}}implicit $r251, implicit $r231
define void @call_external(ptr %out, i64 %value) {
entry:
  call void @external_pair(ptr sret(%pair) align 8 %out, i64 %value)
  ret void
}

; Function-pointer calls follow the identical hidden-result convention. The
; callee address itself is an ordinary incoming argument and does not affect
; the outgoing $251 assignment.
; ISEL-LABEL: name: call_indirect
; ISEL:       $r251 = COPY [[OUT:%[0-9]+]]
; ISEL:       $r231 = COPY [[VALUE:%[0-9]+]]
; ISEL:       CALL_STATE {{.*}}implicit $r251, implicit $r231
define void @call_indirect(ptr %callee, ptr %out, i64 %value) {
entry:
  call void %callee(ptr sret(%pair) align 8 %out, i64 %value)
  ret void
}
