; RUN: not llc -mtriple=mmix %s -o /dev/null 2>&1 | FileCheck %s

target triple = "mmix"

; LLVM's generic VAARG expansion advances by the requested type's allocation
; size and does not implement MMIX's forced big-endian right adjustment or
; aggregate classification. Frontends must expand the frozen cursor protocol.
; CHECK: LLVM ERROR: MMIX does not support raw LLVM va_arg in function 'raw_vaarg'; expand va_list traversal explicitly
define i32 @raw_vaarg(ptr %ap) {
  %value = va_arg ptr %ap, i32
  ret i32 %value
}
