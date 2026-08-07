; RUN: not llc -mtriple=mmix -mattr=-virtual-memory %s -o /dev/null 2>&1 | FileCheck %s

; CHECK: error: llvm.mmix.ldvts: requires the virtual-memory target feature

declare i64 @llvm.mmix.ldvts(i64)

define i64 @virtual_translation_requires_feature(i64 %key) {
  %status = call i64 @llvm.mmix.ldvts(i64 %key)
  ret i64 %status
}
