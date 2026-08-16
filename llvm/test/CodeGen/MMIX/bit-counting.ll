; RUN: llc -mtriple=mmix -O2 < %s | FileCheck %s

declare i64 @llvm.cttz.i64(i64, i1 immarg)
declare i32 @llvm.cttz.i32(i32, i1 immarg)
declare i64 @llvm.ctlz.i64(i64, i1 immarg)

; CHECK-LABEL: cttz64:
; CHECK:       CMPU [[ISZERO:r[0-9]+]], r231, 0
; CHECK:       BZ [[ISZERO]], [[ZERO:.LBB[0-9_]+]]
; CHECK:       SUBU [[LOWER:r[0-9]+]], r231, 1
; CHECK:       ANDN [[MASK:r[0-9]+]], [[LOWER]], r231
; CHECK:       SADD r231, [[MASK]], 0
; CHECK:       [[ZERO]]:
; CHECK:       SETL r231, 64
; CHECK-NOT:   __ctz
define i64 @cttz64(i64 %value) {
  %count = call i64 @llvm.cttz.i64(i64 %value, i1 false)
  ret i64 %count
}

; Narrow integer operations promote to i64 without changing the count for a
; nonzero input.
; CHECK-LABEL: cttz32:
; CHECK:       SUBU [[LOWER:r[0-9]+]], r231, 1
; CHECK:       ANDN [[MASK:r[0-9]+]], [[LOWER]], r231
; CHECK:       SADD r231, [[MASK]], 0
; CHECK-NOT:   __ctz
define i32 @cttz32(i32 %value) {
  %count = call i32 @llvm.cttz.i32(i32 %value, i1 true)
  ret i32 %count
}

; Leading-zero count expands through ordinary integer operations and the
; native population count without acquiring a runtime dependency.
; CHECK-LABEL: ctlz64:
; CHECK:       SADD r231, {{r[0-9]+}}, 0
; CHECK-NOT:   __clz
define i64 @ctlz64(i64 %value) {
  %count = call i64 @llvm.ctlz.i64(i64 %value, i1 false)
  ret i64 %count
}
