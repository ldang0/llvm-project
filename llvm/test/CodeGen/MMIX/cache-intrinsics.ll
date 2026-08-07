; RUN: llc -mtriple=mmix -O0 -verify-machineinstrs -filetype=asm %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=mmix -O0 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefixes=ATTR,MIR
; RUN: llc -mtriple=mmix -O2 -verify-machineinstrs -filetype=asm %s -o - | FileCheck %s --check-prefix=OPT

target triple = "mmix"

declare void @llvm.mmix.preld(ptr, i32 immarg)
declare void @llvm.mmix.prego(ptr, i32 immarg)
declare void @llvm.mmix.prest(ptr, i32 immarg)
declare void @llvm.mmix.syncd(ptr, i32 immarg)
declare void @llvm.mmix.syncid(ptr, i32 immarg)
declare void @llvm.mmix.sync(i32 immarg)
declare void @llvm.prefetch.p0(ptr, i32, i32, i32)

; ATTR: declare void @llvm.mmix.preld(ptr readnone captures(none), i32 immarg) #[[HINT:[0-9]+]]
; ATTR: declare void @llvm.mmix.syncd(ptr captures(none), i32 immarg) #[[RANGE:[0-9]+]]
; ATTR: declare void @llvm.mmix.sync(i32 immarg) #[[SYNC:[0-9]+]]
; ATTR: attributes #[[HINT]] = { noduplicate nounwind memory(inaccessiblemem: readwrite) }
; ATTR: attributes #[[RANGE]] = { noduplicate nounwind memory(argmem: readwrite, inaccessiblemem: readwrite) }
; ATTR: attributes #[[SYNC]] = { noduplicate nounwind }

define void @cache_address_forms(ptr %base, i64 %offset) {
; ASM-LABEL: cache_address_forms:
; ASM:       PRELD 0, {{r[0-9]+}}, 0
; ASM:       PREGO 255, {{r[0-9]+}}, 255
; ASM:       PREST 1, {{r[0-9]+}}, {{r[0-9]+}}
; ASM:       SYNCD 2, {{r[0-9]+}}, {{r[0-9]+}}
; ASM:       SYNCID 3, {{r[0-9]+}}, 0
; MIR-LABEL: name: cache_address_forms
; MIR:       PRELDI 0, %{{[0-9]+}}, 0
; MIR:       PREGOI 255, %{{[0-9]+}}, 255
; MIR:       PREST 1, %{{[0-9]+}}, %{{[0-9]+}}
; MIR:       SYNCD 2, %{{[0-9]+}}, {{(killed )?}}%{{[0-9]+}}
; MIR:       SYNCIDI 3, %{{[0-9]+}}, 0
  call void @llvm.mmix.preld(ptr %base, i32 0)
  %small = getelementptr i8, ptr %base, i64 255
  call void @llvm.mmix.prego(ptr %small, i32 255)
  %dynamic = getelementptr i8, ptr %base, i64 %offset
  call void @llvm.mmix.prest(ptr %dynamic, i32 1)
  %large = getelementptr i8, ptr %base, i64 256
  call void @llvm.mmix.syncd(ptr %large, i32 2)
  call void @llvm.mmix.syncid(ptr %base, i32 3)
  ret void
}

define void @raw_sync_modes() {
; ASM-LABEL: raw_sync_modes:
; ASM:       SYNC 0
; ASM-NEXT:  SYNC 7
; MIR-LABEL: name: raw_sync_modes
; MIR:       SYNC 0
; MIR-NEXT:  SYNC 7
  call void @llvm.mmix.sync(i32 0)
  call void @llvm.mmix.sync(i32 7)
  ret void
}

define void @cache_requests_are_retained(ptr %address) {
; OPT-LABEL: cache_requests_are_retained:
; OPT:       PRELD 7,
; OPT-NEXT:  PRELD 7,
; OPT-NEXT:  PREST 7,
; OPT-NEXT:  PREST 7,
  call void @llvm.mmix.preld(ptr %address, i32 7)
  call void @llvm.mmix.preld(ptr %address, i32 7)
  call void @llvm.mmix.prest(ptr %address, i32 7)
  call void @llvm.mmix.prest(ptr %address, i32 7)
  ret void
}

define void @range_sync_orders_memory(ptr %address) {
; OPT-LABEL: range_sync_orders_memory:
; OPT:       STOU
; OPT:       SYNCD 7,
; OPT:       STOU
; OPT:       SYNCID 7,
; OPT:       STOU
; OPT:       SYNC 3
  store volatile i64 1, ptr %address, align 8
  call void @llvm.mmix.syncd(ptr %address, i32 7)
  store volatile i64 2, ptr %address, align 8
  call void @llvm.mmix.syncid(ptr %address, i32 7)
  store volatile i64 3, ptr %address, align 8
  call void @llvm.mmix.sync(i32 3)
  ret void
}

define void @generic_ir_is_isolated(ptr %address) {
; OPT-LABEL: generic_ir_is_isolated:
; OPT-NOT:   PRELD
; OPT-NOT:   PREGO
; OPT-NOT:   PREST
; OPT-NOT:   SYNCD
; OPT-NOT:   SYNCID
; OPT:       SYNC 3
  call void @llvm.prefetch.p0(ptr %address, i32 0, i32 3, i32 1)
  fence seq_cst
  ret void
}
