; RUN: not --crash llc -mtriple=mmix %s -o /dev/null 2>&1 | FileCheck %s

; CHECK: LLVM ERROR: MMIX does not support indirect branches in the provisional ABI

define void @indirect_branch(ptr %target) {
entry:
  indirectbr ptr %target, [label %destination]

destination:
  ret void
}
