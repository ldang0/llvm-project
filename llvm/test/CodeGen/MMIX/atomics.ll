; RUN: llc -mtriple=mmix -verify-machineinstrs -filetype=asm %s -o - | FileCheck %s
; RUN: llc -mtriple=mmix -verify-machineinstrs -stop-after=postrapseudos %s -o - | FileCheck %s --check-prefix=RP

target triple = "mmix"

; CHECK-LABEL: cmp_monotonic:
; CHECK-NOT:   SYNC
; CHECK:       PUT rP, r232
; CHECK-NEXT:  OR r255, r233, 0
; CHECK-NEXT:  CSWAP r255, r231, 0
; CHECK-NEXT:  GET r231, rP
; CHECK-NOT:   SYNC
; CHECK:       POP 0, 0
; RP-LABEL: name: cmp_monotonic
; RP:       $rp = PUT {{(killed )?}}$r232
; RP-NEXT:  $r255 = ORI killed $r233, 0
; RP-NEXT:  $r255 = CSWAPI killed $r255, {{(killed )?}}$r231, 0, implicit-def $rp, implicit $rp :: (load store monotonic monotonic (s64) on %ir.p)
; RP-NEXT:  $r231 = GET $rp
define i64 @cmp_monotonic(ptr %p, i64 %expected, i64 %new) {
  %pair = cmpxchg ptr %p, i64 %expected, i64 %new monotonic monotonic
  %old = extractvalue { i64, i1 } %pair, 0
  ret i64 %old
}

; CHECK-LABEL: cmp_acquire:
; CHECK-NOT:   SYNC
; CHECK:       CSWAP
; CHECK:       GET
; CHECK:       SYNC 3
; CHECK-NEXT:  POP 0, 0
define i1 @cmp_acquire(ptr %p, i64 %expected, i64 %new) {
  %pair = cmpxchg ptr %p, i64 %expected, i64 %new acquire acquire
  %ok = extractvalue { i64, i1 } %pair, 1
  ret i1 %ok
}

; CHECK-LABEL: cmp_release:
; CHECK:       SYNC 3
; CHECK:       CSWAP
; CHECK-NOT:   SYNC
; CHECK:       POP 0, 0
define i1 @cmp_release(ptr %p, i64 %expected, i64 %new) {
  %pair = cmpxchg ptr %p, i64 %expected, i64 %new release monotonic
  %ok = extractvalue { i64, i1 } %pair, 1
  ret i1 %ok
}

; CHECK-LABEL: cmp_acqrel:
; CHECK:       SYNC 3
; CHECK:       CSWAP
; CHECK:       SYNC 3
; CHECK-NEXT:  POP 0, 0
define i1 @cmp_acqrel(ptr %p, i64 %expected, i64 %new) {
  %pair = cmpxchg ptr %p, i64 %expected, i64 %new acq_rel acquire
  %ok = extractvalue { i64, i1 } %pair, 1
  ret i1 %ok
}

; CHECK-LABEL: cmp_seqcst:
; CHECK:       SYNC 3
; CHECK:       CSWAP
; CHECK:       SYNC 3
; CHECK-NEXT:  POP 0, 0
define i1 @cmp_seqcst(ptr %p, i64 %expected, i64 %new) {
  %pair = cmpxchg ptr %p, i64 %expected, i64 %new seq_cst seq_cst
  %ok = extractvalue { i64, i1 } %pair, 1
  ret i1 %ok
}

; Atomic RMW operations expand to a retrying cmpxchg loop.
; CHECK-LABEL: rmw_add:
; CHECK:       SYNC 3
; CHECK:       CSWAP
; CHECK:       ADDU
; CHECK:       CSWAP
; CHECK:       BNZB
; CHECK:       SYNC 3
define i64 @rmw_add(ptr %p, i64 %value) {
  %old = atomicrmw add ptr %p, i64 %value acq_rel
  ret i64 %old
}

; Sub-octabyte operations use a big-endian masked octabyte cmpxchg loop.
; CHECK-LABEL: rmw_add_i8:
; CHECK:       ANDN
; CHECK:       CSWAP
; CHECK:       AND
; CHECK:       CSWAP
; CHECK:       BNZB
define i8 @rmw_add_i8(ptr %p, i8 %value) {
  %old = atomicrmw add ptr %p, i8 %value monotonic
  ret i8 %old
}

; Atomic loads use a no-change cmpxchg and acquire ordering is a trailing
; memory fence.
; CHECK-LABEL: load_acquire:
; CHECK:       PUT rP
; CHECK-NEXT:  OR r255
; CHECK-NEXT:  CSWAP
; CHECK-NEXT:  GET
; CHECK-NEXT:  SYNC 3
define i64 @load_acquire(ptr %p) {
  %value = load atomic i64, ptr %p acquire, align 8
  ret i64 %value
}

; Atomic stores use a retrying exchange loop and release ordering is a leading
; memory fence.
; CHECK-LABEL: store_release:
; CHECK:       SYNC 3
; CHECK:       CSWAP
; CHECK:       CSWAP
; CHECK:       BNZB
define void @store_release(ptr %p, i64 %value) {
  store atomic i64 %value, ptr %p release, align 8
  ret void
}

; CHECK-LABEL: fences:
; CHECK:       SYNC 3
; CHECK-NEXT:  SYNC 3
; CHECK-NEXT:  SYNC 3
; CHECK-NEXT:  SYNC 3
; CHECK-NEXT:  POP 0, 0
define void @fences() {
  fence acquire
  fence release
  fence acq_rel
  fence seq_cst
  ret void
}
