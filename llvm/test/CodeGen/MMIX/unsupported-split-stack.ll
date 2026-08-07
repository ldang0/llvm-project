; RUN: not --crash llc -mtriple=mmix %s -o /dev/null 2>&1 | FileCheck %s

target triple = "mmix"

; CHECK: LLVM ERROR: MMIX does not support split stacks
define void @split_stack() #0 {
  %slot = alloca i64, align 8
  store volatile i64 0, ptr %slot, align 8
  ret void
}

attributes #0 = { "split-stack" }
