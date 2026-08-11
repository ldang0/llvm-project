; RUN: not llc -mtriple=mmix -filetype=asm %s -o /dev/null 2>&1 | FileCheck %s

; CHECK: LLVM ERROR: MMIX does not support va_start in function 'unsupported_varargs'

declare void @llvm.va_start(ptr)

define void @unsupported_varargs(i64 %fixed, ...) {
  %ap = alloca ptr, align 8
  call void @llvm.va_start(ptr %ap)
  ret void
}
