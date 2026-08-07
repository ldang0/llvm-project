; RUN: not --crash llc -mtriple=mmix %s -o /dev/null 2>&1 | FileCheck %s

; CHECK: LLVM ERROR: unsupported library call operation

define void @add_f16(ptr %out, ptr %lhs, ptr %rhs) {
  %lhs.value = load half, ptr %lhs, align 2
  %rhs.value = load half, ptr %rhs, align 2
  %sum = fadd half %lhs.value, %rhs.value
  store half %sum, ptr %out, align 2
  ret void
}
