; RUN: llc -mtriple=mmix -O0 -verify-machineinstrs -filetype=asm %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=mmix -O0 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefixes=ATTR,MIR
; RUN: llc -mtriple=mmix -O2 -verify-machineinstrs -filetype=asm %s -o - | FileCheck %s --check-prefix=OPT

target triple = "mmix"

declare i64 @llvm.mmix.ldvts(i64)
declare void @llvm.mmix.put(i32 immarg, i64)
declare void @llvm.mmix.sync(i32 immarg)

; ATTR: declare noundef range(i64 0, 4) i64 @llvm.mmix.ldvts(i64) #[[LDVTS_ATTRS:[0-9]+]]
; ATTR: attributes #[[LDVTS_ATTRS]] = { noduplicate nounwind memory(inaccessiblemem: readwrite) }

define i64 @virtual_translation_address_forms(i64 %base, i64 %offset) {
; ASM-LABEL: virtual_translation_address_forms:
; ASM:       LDVTS {{r[0-9]+}}, {{r[0-9]+}}, 0
; ASM:       LDVTS {{r[0-9]+}}, {{r[0-9]+}}, 255
; ASM:       LDVTS {{r[0-9]+}}, {{r[0-9]+}}, {{r[0-9]+}}
; ASM:       LDVTS {{r[0-9]+}}, {{r[0-9]+}}, {{r[0-9]+}}
; MIR-LABEL: name: virtual_translation_address_forms
; MIR:       LDVTSI %{{[0-9]+}}, 0
; MIR:       LDVTSI %{{[0-9]+}}, 255
; MIR:       LDVTS %{{[0-9]+}}, {{(killed )?}}%{{[0-9]+}}
; MIR:       LDVTS %{{[0-9]+}}, {{(killed )?}}%{{[0-9]+}}
  %small = add i64 %base, 255
  %dynamic = add i64 %base, %offset
  %large = add i64 %base, 256
  %a = call i64 @llvm.mmix.ldvts(i64 %base)
  %b = call i64 @llvm.mmix.ldvts(i64 %small)
  %c = call i64 @llvm.mmix.ldvts(i64 %dynamic)
  %d = call i64 @llvm.mmix.ldvts(i64 %large)
  %ab = xor i64 %a, %b
  %abc = xor i64 %ab, %c
  %abcd = xor i64 %abc, %d
  ret i64 %abcd
}

define i64 @virtual_translation_requests_are_retained(i64 %key) {
; OPT-LABEL: virtual_translation_requests_are_retained:
; OPT:       LDVTS
; OPT-NEXT:  LDVTS
; OPT-NEXT:  LDVTS
  %dead = call i64 @llvm.mmix.ldvts(i64 %key)
  %first = call i64 @llvm.mmix.ldvts(i64 %key)
  %second = call i64 @llvm.mmix.ldvts(i64 %key)
  %result = xor i64 %first, %second
  ret i64 %result
}

define i64 @virtual_translation_state_ordering(i64 %rv, i64 %key) {
; OPT-LABEL: virtual_translation_state_ordering:
; OPT:       PUT rV,
; OPT-NEXT:  LDVTS
; OPT-NEXT:  SYNC 6
; OPT-NEXT:  LDVTS
  call void @llvm.mmix.put(i32 18, i64 %rv)
  %before = call i64 @llvm.mmix.ldvts(i64 %key)
  call void @llvm.mmix.sync(i32 6)
  %after = call i64 @llvm.mmix.ldvts(i64 %key)
  %result = xor i64 %before, %after
  ret i64 %result
}

define i64 @generic_ir_is_isolated(ptr %address, i64 %key) {
; ASM-LABEL: generic_ir_is_isolated:
; ASM-NOT:   LDVTS
; ASM:       LDOU
; ASM:       ADDU
  %loaded = load volatile i64, ptr %address, align 8
  %result = add i64 %loaded, %key
  ret i64 %result
}
