; RUN: not --crash llc -mtriple=mmix %s -o /dev/null 2>&1 | FileCheck %s

; CHECK: LLVM ERROR: MMIX constrained floating-point lowering is not implemented

define float @strict_add_f32(float %lhs, float %rhs) strictfp {
  %result = call float @llvm.experimental.constrained.fadd.f32(
      float %lhs, float %rhs, metadata !"round.dynamic",
      metadata !"fpexcept.strict")
  ret float %result
}

declare float @llvm.experimental.constrained.fadd.f32(
    float, float, metadata, metadata)
