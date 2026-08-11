; RUN: not llc -mtriple=mmix -filetype=asm %s -o %t.s 2>&1 | FileCheck %s
; RUN: test ! -s %t.s
; RUN: not llc -mtriple=mmix -filetype=obj %s -o %t.o 2>&1 | FileCheck %s
; RUN: test ! -s %t.o

target triple = "mmix"

; CHECK: LLVM ERROR: MMIX does not support split stacks in function 'split_stack'
define void @split_stack() #0 {
  %slot = alloca i64, align 8
  store volatile i64 0, ptr %slot, align 8
  ret void
}

attributes #0 = { "split-stack" }
