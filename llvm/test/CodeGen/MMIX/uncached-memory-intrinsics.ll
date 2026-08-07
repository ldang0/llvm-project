; RUN: llc -mtriple=mmix -O0 -verify-machineinstrs -filetype=asm %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=mmix -O0 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefixes=ATTR,MIR
; RUN: llc -mtriple=mmix -O2 -verify-machineinstrs -filetype=asm %s -o - | FileCheck %s --check-prefix=OPT

target triple = "mmix"

declare i64 @llvm.mmix.ldunc(ptr)
declare void @llvm.mmix.stunc(ptr, i64)

; ATTR: declare i64 @llvm.mmix.ldunc(ptr readonly align 8 captures(none)) #[[LDUNC_ATTRS:[0-9]+]]
; ATTR: declare void @llvm.mmix.stunc(ptr writeonly align 8 captures(none), i64) #[[STUNC_ATTRS:[0-9]+]]
; ATTR: attributes #[[LDUNC_ATTRS]] = { noduplicate nounwind memory(argmem: read, inaccessiblemem: readwrite) }
; ATTR: attributes #[[STUNC_ATTRS]] = { noduplicate nounwind memory(argmem: write, inaccessiblemem: readwrite) }

define i64 @uncached_load_address_forms(ptr %base, i64 %offset) {
; ASM-LABEL: uncached_load_address_forms:
; ASM:       LDUNC {{r[0-9]+}}, {{r[0-9]+}}, 0
; ASM:       LDUNC {{r[0-9]+}}, {{r[0-9]+}}, 248
; ASM:       LDUNC {{r[0-9]+}}, {{r[0-9]+}}, {{r[0-9]+}}
; ASM:       LDUNC {{r[0-9]+}}, {{r[0-9]+}}, {{r[0-9]+}}
; MIR-LABEL: name: uncached_load_address_forms
; MIR:       LDUNCI %{{[0-9]+}}, 0 :: (load (s64) from %ir.base)
; MIR:       LDUNCI %{{[0-9]+}}, 248 :: (load (s64) from %ir.small)
; MIR:       LDUNC %{{[0-9]+}}, {{(killed )?}}%{{[0-9]+}} :: (load (s64) from %ir.dynamic)
; MIR:       LDUNC %{{[0-9]+}}, {{(killed )?}}%{{[0-9]+}} :: (load (s64) from %ir.large)
  %small = getelementptr i8, ptr %base, i64 248
  %scaled = shl i64 %offset, 3
  %dynamic = getelementptr i8, ptr %base, i64 %scaled
  %large = getelementptr i8, ptr %base, i64 256
  %a = call i64 @llvm.mmix.ldunc(ptr %base)
  %b = call i64 @llvm.mmix.ldunc(ptr %small)
  %c = call i64 @llvm.mmix.ldunc(ptr %dynamic)
  %d = call i64 @llvm.mmix.ldunc(ptr %large)
  %ab = xor i64 %a, %b
  %abc = xor i64 %ab, %c
  %abcd = xor i64 %abc, %d
  ret i64 %abcd
}

define void @uncached_store_address_forms(ptr %base, i64 %offset, i64 %value) {
; ASM-LABEL: uncached_store_address_forms:
; ASM:       STUNC {{r[0-9]+}}, {{r[0-9]+}}, 0
; ASM:       STUNC {{r[0-9]+}}, {{r[0-9]+}}, 248
; ASM:       STUNC {{r[0-9]+}}, {{r[0-9]+}}, {{r[0-9]+}}
; ASM:       STUNC {{r[0-9]+}}, {{r[0-9]+}}, {{r[0-9]+}}
; MIR-LABEL: name: uncached_store_address_forms
; MIR:       STUNCI %{{[0-9]+}}, %{{[0-9]+}}, 0 :: (store (s64) into %ir.base)
; MIR:       STUNCI %{{[0-9]+}}, %{{[0-9]+}}, 248 :: (store (s64) into %ir.small)
; MIR:       STUNC %{{[0-9]+}}, %{{[0-9]+}}, {{(killed )?}}%{{[0-9]+}} :: (store (s64) into %ir.dynamic)
; MIR:       STUNC %{{[0-9]+}}, %{{[0-9]+}}, {{(killed )?}}%{{[0-9]+}} :: (store (s64) into %ir.large)
  %small = getelementptr i8, ptr %base, i64 248
  %scaled = shl i64 %offset, 3
  %dynamic = getelementptr i8, ptr %base, i64 %scaled
  %large = getelementptr i8, ptr %base, i64 256
  call void @llvm.mmix.stunc(ptr %base, i64 %value)
  call void @llvm.mmix.stunc(ptr %small, i64 %value)
  call void @llvm.mmix.stunc(ptr %dynamic, i64 %value)
  call void @llvm.mmix.stunc(ptr %large, i64 %value)
  ret void
}

define i64 @uncached_alias_metadata(ptr %address, i64 %value) {
; MIR-LABEL: name: uncached_alias_metadata
; MIR:       LDUNCI %{{[0-9]+}}, 0 :: (load (s64) from %ir.address, !alias.scope !{{[0-9]+}}, !noalias !{{[0-9]+}})
; MIR:       STUNCI %{{[0-9]+}}, %{{[0-9]+}}, 0 :: (store (s64) into %ir.address, !alias.scope !{{[0-9]+}}, !noalias !{{[0-9]+}})
  %loaded = call i64 @llvm.mmix.ldunc(ptr %address), !alias.scope !2, !noalias !5
  call void @llvm.mmix.stunc(ptr %address, i64 %value), !alias.scope !5, !noalias !2
  ret i64 %loaded
}

define void @uncached_requests_are_retained(ptr %address, i64 %value) {
; OPT-LABEL: uncached_requests_are_retained:
; OPT:       LDUNC
; OPT-NEXT:  LDUNC
; OPT-NEXT:  STUNC
; OPT-NEXT:  STUNC
  %dead0 = call i64 @llvm.mmix.ldunc(ptr %address)
  %dead1 = call i64 @llvm.mmix.ldunc(ptr %address)
  call void @llvm.mmix.stunc(ptr %address, i64 %value)
  call void @llvm.mmix.stunc(ptr %address, i64 %value)
  ret void
}

define void @uncached_accesses_order_memory(ptr %address, i64 %value) {
; OPT-LABEL: uncached_accesses_order_memory:
; OPT:       STOU
; OPT:       LDUNC
; OPT:       STOU
; OPT:       STUNC
; OPT:       LDOU
  store volatile i64 1, ptr %address, align 8
  %dead = call i64 @llvm.mmix.ldunc(ptr %address)
  store volatile i64 2, ptr %address, align 8
  call void @llvm.mmix.stunc(ptr %address, i64 %value)
  %observed = load volatile i64, ptr %address, align 8
  ret void
}

define i64 @generic_accesses_are_cached(ptr %address, i64 %value) {
; ASM-LABEL: generic_accesses_are_cached:
; ASM-NOT:   LDUNC
; ASM-NOT:   STUNC
; ASM:       LDOU
; ASM:       LDOU
; ASM:       STOU
; ASM:       STOU
  %ordinary = load i64, ptr %address, align 8
  %volatile = load volatile i64, ptr %address, align 8
  store i64 %value, ptr %address, align 8
  store volatile i64 %value, ptr %address, align 8
  %result = xor i64 %ordinary, %volatile
  ret i64 %result
}

!0 = distinct !{!0}
!1 = distinct !{!1, !0}
!2 = !{!1}
!3 = distinct !{!3}
!4 = distinct !{!4, !3}
!5 = !{!4}
