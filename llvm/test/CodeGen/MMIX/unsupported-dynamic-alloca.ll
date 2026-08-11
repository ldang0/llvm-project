; RUN: not llc -mtriple=mmix -filetype=asm %s -o %t.s 2>&1 | FileCheck %s
; RUN: test ! -s %t.s
; RUN: not llc -mtriple=mmix -filetype=obj %s -o %t.o 2>&1 | FileCheck %s
; RUN: test ! -s %t.o

; CHECK: LLVM ERROR: MMIX does not support dynamic stack allocation in function 'dynamic_alloca'

define i64 @dynamic_alloca(i64 %count) {
  %storage = alloca i64, i64 %count, align 8
  store volatile i64 1, ptr %storage
  %value = load volatile i64, ptr %storage
  ret i64 %value
}
