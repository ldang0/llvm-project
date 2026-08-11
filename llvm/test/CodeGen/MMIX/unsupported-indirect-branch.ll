; RUN: not llc -mtriple=mmix -filetype=asm %s -o %t.s 2>&1 | FileCheck %s
; RUN: test ! -s %t.s
; RUN: not llc -mtriple=mmix -filetype=obj %s -o %t.o 2>&1 | FileCheck %s
; RUN: test ! -s %t.o

; CHECK: LLVM ERROR: MMIX does not support indirect branches in ordinary function 'indirect_branch'

define void @indirect_branch(ptr %target) {
entry:
  indirectbr ptr %target, [label %destination]

destination:
  ret void
}
