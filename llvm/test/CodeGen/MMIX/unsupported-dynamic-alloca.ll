; RUN: not llc -mtriple=mmix %s -o /dev/null 2>&1 | FileCheck %s

; CHECK: LLVM ERROR: MMIX does not support dynamic stack allocation

define i64 @dynamic_alloca(i64 %count) {
  %storage = alloca i64, i64 %count, align 8
  store volatile i64 1, ptr %storage
  %value = load volatile i64, ptr %storage
  ret i64 %value
}
