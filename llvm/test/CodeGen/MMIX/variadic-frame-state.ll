; RUN: llc -mtriple=mmix -verify-machineinstrs -stop-after=mmix-isel %s -o - \
; RUN:   | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=mmix -verify-machineinstrs -stop-after=prolog-epilog %s -o - \
; RUN:   | FileCheck %s --check-prefix=PEI

target triple = "mmix"

%large = type { i64, i64 }

declare void @stack_callee(i64, i64, i64, i64, i64, i64, i64, i64,
                           i64, i64, i64, i64, i64, i64, i64, i64, i64)

; With K=0, all sixteen incoming argument registers belong to the contiguous
; save area immediately below the incoming stack pointer.
; ISEL-LABEL: name: k0
; ISEL:       fixedStack:
; ISEL-NEXT:    - { id: 0, {{.*}}offset: -128, size: 128, alignment: 8,
; ISEL-NEXT:      isImmutable: false
define void @k0(...) {
  ret void
}

; K=15 leaves only $246 in the register portion of the unnamed sequence.
; ISEL-LABEL: name: k15
; ISEL:       fixedStack:
; ISEL-NEXT:    - { id: 0, {{.*}}offset: -8, size: 8, alignment: 8,
; ISEL-NEXT:      isImmutable: false
define void @k15(i64, i64, i64, i64, i64, i64, i64, i64,
                 i64, i64, i64, i64, i64, i64, i64, ...) {
  ret void
}

; The dedicated $251 indirect-result pointer is not an ordinary named slot.
; ISEL-LABEL: name: k15_sret
; ISEL:       fixedStack:
; ISEL-NEXT:    - { id: 0, {{.*}}offset: -8, size: 8, alignment: 8,
; ISEL-NEXT:      isImmutable: false
define void @k15_sret(ptr sret(%large) align 8, i64, i64, i64, i64, i64,
                      i64, i64, i64, i64, i64, i64, i64, i64, i64, i64,
                      ...) {
  ret void
}

; K=16 uses every argument register, so the first unnamed address is the
; first incoming stack slot and there is no writable register-save object.
; ISEL-LABEL: name: k16
; ISEL:       fixedStack:
; ISEL-NEXT:    - { id: 0, {{.*}}offset: 0, size: 8, alignment: 8,
; ISEL-NEXT:      isImmutable: true
define void @k16(i64, i64, i64, i64, i64, i64, i64, i64,
                 i64, i64, i64, i64, i64, i64, i64, i64, ...) {
  ret void
}

; At K=17, the named stack argument remains at S while the first unnamed
; address advances to the next octa at S+8.
; ISEL-LABEL: name: k17
; ISEL:       fixedStack:
; ISEL-NEXT:    - { id: 0, {{.*}}offset: 0, size: 8, alignment: 8,
; ISEL-NEXT:      isImmutable: true
; ISEL:         - { id: 1, {{.*}}offset: 8, size: 8, alignment: 8,
; ISEL-NEXT:      isImmutable: true
define i64 @k17(i64, i64, i64, i64, i64, i64, i64, i64,
                i64, i64, i64, i64, i64, i64, i64, i64, i64 %last, ...) {
  ret i64 %last
}

; The fixed register-save area occupies [-128, 0) relative to incoming SP.
; PEI places the saved frame pointer, ordinary local, and reserved outgoing
; call area below it without changing any fixed offset.
; PEI-LABEL: name: k0_with_frame
; PEI:       stackSize: 152
; PEI:       maxCallFrameSize: 8
; PEI:       fixedStack:
; PEI:         offset: -128, size: 128
; PEI:       stack:
; PEI-DAG:     - { id: 0, {{.*}}offset: -144, size: 8, alignment: 8,
; PEI-DAG:     - { id: 1, {{.*}}offset: -136, size: 8, alignment: 8,
; PEI:       $r254 = frame-setup SUBUI $r254, 152
; PEI:       STOUI killed $r253, $r254, 16
; PEI:       $r253 = frame-setup ADDUI $r254, 152
; PEI-NOT:   ADJCALLSTACK
; PEI:       DIRECT_CALL_STATE @stack_callee
define void @k0_with_frame(...) nounwind #0 {
  %local = alloca i64, align 8
  store volatile i64 1, ptr %local, align 8
  call void @stack_callee(i64 0, i64 1, i64 2, i64 3, i64 4, i64 5,
                          i64 6, i64 7, i64 8, i64 9, i64 10, i64 11,
                          i64 12, i64 13, i64 14, i64 15, i64 16)
  ret void
}

attributes #0 = { "frame-pointer"="all" }
