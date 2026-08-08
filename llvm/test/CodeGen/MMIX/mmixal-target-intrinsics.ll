; RUN: llc -mtriple=mmix-unknown-elf -filetype=asm -O0 \
; RUN:   -verify-machineinstrs --output-asm-variant=1 %s -o - \
; RUN:   | FileCheck %s

target triple = "mmix-unknown-elf"

declare i64 @llvm.mmix.get(i32 immarg)
declare void @llvm.mmix.put(i32 immarg, i64)
declare void @llvm.mmix.preld(ptr, i32 immarg)
declare void @llvm.mmix.prego(ptr, i32 immarg)
declare void @llvm.mmix.prest(ptr, i32 immarg)
declare void @llvm.mmix.syncd(ptr, i32 immarg)
declare void @llvm.mmix.syncid(ptr, i32 immarg)
declare void @llvm.mmix.sync(i32 immarg)
declare i64 @llvm.mmix.ldunc(ptr)
declare void @llvm.mmix.stunc(ptr, i64)
declare i64 @llvm.mmix.ldvts(i64)

define void @Main() {
entry:
  br label %loop

loop:
  br label %loop
}

define void @cache_and_range_sync(ptr %base, i64 %offset) noinline {
; CHECK-LABEL: cache_and_range_sync IS @
; CHECK:       PRELD 0, ${{[0-9]+}}, 0
; CHECK:       PREGO 255, ${{[0-9]+}}, 255
; CHECK:       PREST 1, ${{[0-9]+}}, ${{[0-9]+}}
; CHECK:       SYNCD 2, ${{[0-9]+}}, ${{[0-9]+}}
; CHECK:       SYNCID 3, ${{[0-9]+}}, 0
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

; These checks establish source syntax and intrinsic lowering only. Runtime
; privilege for individual modes is outside this compile-time contract.
define void @raw_sync() noinline {
; CHECK-LABEL: raw_sync IS @
; CHECK:       SYNC 0
; CHECK-NEXT:  SYNC 7
  call void @llvm.mmix.sync(i32 0)
  call void @llvm.mmix.sync(i32 7)
  ret void
}

define i64 @uncached_memory(ptr %base, i64 %offset, i64 %value) noinline {
; CHECK-LABEL: uncached_memory IS @
; CHECK:       LDUNC ${{[0-9]+}}, ${{[0-9]+}}, 0
; CHECK:       STUNC ${{[0-9]+}}, ${{[0-9]+}}, ${{[0-9]+}}
  %loaded = call i64 @llvm.mmix.ldunc(ptr %base)
  %dynamic = getelementptr i8, ptr %base, i64 %offset
  call void @llvm.mmix.stunc(ptr %dynamic, i64 %value)
  ret i64 %loaded
}

; LDVTS is accepted as an architectural operation; this test does not assert
; that the eventual execution environment grants the required privilege.
define i64 @virtual_translation(i64 %base, i64 %offset) noinline {
; CHECK-LABEL: virtual_translation IS @
; CHECK:       LDVTS ${{[0-9]+}}, ${{[0-9]+}}, ${{[0-9]+}}
  %key = add i64 %base, %offset
  %result = call i64 @llvm.mmix.ldvts(i64 %key)
  ret i64 %result
}

define i64 @special_registers(i64 %value) noinline {
; CHECK-LABEL: special_registers IS @
; CHECK:       GET ${{[0-9]+}}, rB
; CHECK-NEXT:  GET ${{[0-9]+}}, rZZ
; CHECK:       PUT rH, 255
; CHECK-NEXT:  PUT rH, ${{[0-9]+}}
; CHECK:       PUT rC, 1
; CHECK-NEXT:  PUT rV, 2
  %b = call i64 @llvm.mmix.get(i32 0)
  %zz = call i64 @llvm.mmix.get(i32 31)
  call void @llvm.mmix.put(i32 3, i64 255)
  call void @llvm.mmix.put(i32 3, i64 %value)
  call void @llvm.mmix.put(i32 8, i64 1)
  call void @llvm.mmix.put(i32 18, i64 2)
  %result = xor i64 %b, %zz
  ret i64 %result
}
