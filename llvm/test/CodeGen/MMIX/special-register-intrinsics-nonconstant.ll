; RUN: not llc -mtriple=mmix %s -o /dev/null 2>&1 | FileCheck %s

; CHECK: immarg operand has non-immediate parameter

declare i64 @llvm.mmix.get(i32 immarg)
declare void @llvm.mmix.put(i32 immarg, i64)

define i64 @get_nonconstant(i32 %selector) {
  %value = call i64 @llvm.mmix.get(i32 %selector)
  ret i64 %value
}

define void @put_nonconstant(i32 %selector, i64 %value) {
  call void @llvm.mmix.put(i32 %selector, i64 %value)
  ret void
}
