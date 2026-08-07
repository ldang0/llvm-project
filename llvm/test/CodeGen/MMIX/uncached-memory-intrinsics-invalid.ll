; RUN: not llc -mtriple=mmix -mattr=-cache %s -o /dev/null 2>&1 | FileCheck %s

; CHECK-DAG: error: llvm.mmix.ldunc: requires the cache target feature
; CHECK-DAG: error: llvm.mmix.stunc: requires the cache target feature

declare i64 @llvm.mmix.ldunc(ptr)
declare void @llvm.mmix.stunc(ptr, i64)

define i64 @uncached_load_requires_cache(ptr %address) {
  %value = call i64 @llvm.mmix.ldunc(ptr %address)
  ret i64 %value
}

define void @uncached_store_requires_cache(ptr %address, i64 %value) {
  call void @llvm.mmix.stunc(ptr %address, i64 %value)
  ret void
}
