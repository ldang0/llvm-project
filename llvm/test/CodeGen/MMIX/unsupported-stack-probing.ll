; RUN: not llc -mtriple=mmix -filetype=asm %s -o %t.s 2>&1 | FileCheck %s
; RUN: test ! -s %t.s
; RUN: not llc -mtriple=mmix -filetype=obj %s -o %t.o 2>&1 | FileCheck %s
; RUN: test ! -s %t.o

target triple = "mmix"

; CHECK: LLVM ERROR: MMIX does not support stack probing in function 'stack_probe'
define void @stack_probe() #0 {
  %slot = alloca i64, align 8
  store volatile i64 0, ptr %slot, align 8
  ret void
}

attributes #0 = { "probe-stack"="inline-asm" }
