; RUN: split-file %s %t
; RUN: not llc -mtriple=mmix -filetype=asm %t/overaligned.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=OVERALIGNED
; RUN: not --crash llc -mtriple=mmix -filetype=asm %t/address-space.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ADDRESS-SPACE

; OVERALIGNED: MMIX does not support aggregate or special formal arguments in function 'overaligned'
; ADDRESS-SPACE: MMIX does not support nonzero-address-space formal arguments

;--- overaligned.ll
target triple = "mmix"

%pair = type { i64, i64 }

define void @overaligned(ptr byval(%pair) align 16 %value) {
  ret void
}

;--- address-space.ll
target triple = "mmix"

%pair = type { i64, i64 }

define void @nonzero_address_space(
    ptr addrspace(1) byval(%pair) align 8 %value) {
  ret void
}
