; RUN: not --crash llc -mtriple=mmix %s -o /dev/null 2>&1 | FileCheck %s

; CHECK: LLVM ERROR: unsupported library call operation

define void @add_f32(ptr %out, ptr %lhs, ptr %rhs) {
  %lhs.value = load float, ptr %lhs, align 4
  %rhs.value = load float, ptr %rhs, align 4
  %sum = fadd float %lhs.value, %rhs.value
  store float %sum, ptr %out, align 4
  ret void
}
