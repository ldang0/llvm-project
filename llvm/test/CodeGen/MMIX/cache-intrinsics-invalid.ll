; RUN: not llc -mtriple=mmix %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=RANGE

; RANGE-DAG: error: llvm.mmix.preld: span must be in the range [0, 255]
; RANGE-DAG: error: llvm.mmix.prego: span must be in the range [0, 255]
; RANGE-DAG: error: llvm.mmix.prest: span must be in the range [0, 255]
; RANGE-DAG: error: llvm.mmix.syncd: span must be in the range [0, 255]
; RANGE-DAG: error: llvm.mmix.syncid: span must be in the range [0, 255]
; RANGE: error: llvm.mmix.sync: mode must be in the range [0, 7]
; RANGE: error: llvm.mmix.sync: mode must be in the range [0, 7]

declare void @llvm.mmix.preld(ptr, i32 immarg)
declare void @llvm.mmix.prego(ptr, i32 immarg)
declare void @llvm.mmix.prest(ptr, i32 immarg)
declare void @llvm.mmix.syncd(ptr, i32 immarg)
declare void @llvm.mmix.syncid(ptr, i32 immarg)
declare void @llvm.mmix.sync(i32 immarg)

define void @preld_span_negative(ptr %address) {
  call void @llvm.mmix.preld(ptr %address, i32 -1)
  ret void
}

define void @prego_span_large(ptr %address) {
  call void @llvm.mmix.prego(ptr %address, i32 256)
  ret void
}

define void @prest_span_negative(ptr %address) {
  call void @llvm.mmix.prest(ptr %address, i32 -1)
  ret void
}

define void @syncd_span_large(ptr %address) {
  call void @llvm.mmix.syncd(ptr %address, i32 256)
  ret void
}

define void @syncid_span_large(ptr %address) {
  call void @llvm.mmix.syncid(ptr %address, i32 256)
  ret void
}

define void @mode_negative() {
  call void @llvm.mmix.sync(i32 -1)
  ret void
}

define void @mode_large() {
  call void @llvm.mmix.sync(i32 8)
  ret void
}
