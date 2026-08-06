; RUN: not llc -mtriple=mmix -O0 < %s -o /dev/null 2>&1 | FileCheck %s

target triple = "mmix"

define void @memory_operand(ptr %p) {
; CHECK: error: unknown asm constraint 'm'
  call void asm sideeffect "LDO r0, $0, 0", "m"(ptr %p)
  ret void
}
