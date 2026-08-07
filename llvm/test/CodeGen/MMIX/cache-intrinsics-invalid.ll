; RUN: not llc -mtriple=mmix %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=RANGE
; RUN: not llc -mtriple=mmix -mattr=-cache,-system %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=FEATURE

; RANGE: error: llvm.mmix.preld: span must be in the range [0, 255]
; RANGE: error: llvm.mmix.syncid: span must be in the range [0, 255]
; RANGE: error: llvm.mmix.sync: mode must be in the range [0, 7]
; RANGE: error: llvm.mmix.sync: mode must be in the range [0, 7]
; FEATURE-DAG: error: llvm.mmix.preld: requires the cache target feature
; FEATURE-DAG: error: llvm.mmix.sync: requires the system target feature

declare void @llvm.mmix.preld(ptr, i32 immarg)
declare void @llvm.mmix.syncid(ptr, i32 immarg)
declare void @llvm.mmix.sync(i32 immarg)

define void @span_negative(ptr %address) {
  call void @llvm.mmix.preld(ptr %address, i32 -1)
  ret void
}

define void @span_large(ptr %address) {
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

