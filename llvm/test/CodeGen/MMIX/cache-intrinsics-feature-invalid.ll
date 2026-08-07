; RUN: not llc -mtriple=mmix -mattr=-cache,-system %s -o /dev/null 2>&1 | FileCheck %s

; CHECK-DAG: error: llvm.mmix.preld: requires the cache target feature
; CHECK-DAG: error: llvm.mmix.prego: requires the cache target feature
; CHECK-DAG: error: llvm.mmix.prest: requires the cache target feature
; CHECK-DAG: error: llvm.mmix.syncd: requires the cache target feature
; CHECK-DAG: error: llvm.mmix.syncid: requires the cache target feature
; CHECK-DAG: error: llvm.mmix.sync: requires the system target feature

declare void @llvm.mmix.preld(ptr, i32 immarg)
declare void @llvm.mmix.prego(ptr, i32 immarg)
declare void @llvm.mmix.prest(ptr, i32 immarg)
declare void @llvm.mmix.syncd(ptr, i32 immarg)
declare void @llvm.mmix.syncid(ptr, i32 immarg)
declare void @llvm.mmix.sync(i32 immarg)

define void @cache_intrinsics_require_cache(ptr %address) {
  call void @llvm.mmix.preld(ptr %address, i32 0)
  call void @llvm.mmix.prego(ptr %address, i32 1)
  call void @llvm.mmix.prest(ptr %address, i32 2)
  call void @llvm.mmix.syncd(ptr %address, i32 3)
  call void @llvm.mmix.syncid(ptr %address, i32 4)
  ret void
}

define void @sync_requires_system() {
  call void @llvm.mmix.sync(i32 3)
  ret void
}
