; RUN: split-file %s %t
; RUN: not llc -mtriple=mmix -O0 %t/non-offsettable.ll -o /dev/null 2>&1 \
; RUN:   | FileCheck %s --check-prefix=NON-OFFSETTABLE
; RUN: not llc -mtriple=mmix -O0 %t/address-space.ll -o /dev/null 2>&1 \
; RUN:   | FileCheck %s --check-prefix=ADDRESS-SPACE

; NON-OFFSETTABLE: LLVM ERROR: MMIX has no non-offsettable inline assembly memory operand
; ADDRESS-SPACE: LLVM ERROR: MMIX inline assembly does not support memory or address operands in nonzero address spaces

;--- non-offsettable.ll
target triple = "mmix"

define void @non_offsettable_memory(ptr %p) {
  call void asm sideeffect "LDO r0, $0", "*V"(ptr elementtype(i64) %p)
  ret void
}

;--- address-space.ll
target triple = "mmix"

@object = addrspace(1) global i64 0

define void @nonzero_address_space() {
  call void asm sideeffect "LDO r0, $0", "*m"(
      ptr addrspace(1) elementtype(i64) @object)
  ret void
}
