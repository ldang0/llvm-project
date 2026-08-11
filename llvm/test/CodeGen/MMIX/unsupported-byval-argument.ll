; RUN: split-file %s %t
; RUN: not llc -mtriple=mmix -filetype=asm %t/overaligned.ll -o %t/overaligned.s 2>&1 | FileCheck %s --check-prefix=OVERALIGNED
; RUN: test ! -s %t/overaligned.s
; RUN: not llc -mtriple=mmix -filetype=obj %t/overaligned.ll -o %t/overaligned.o 2>&1 | FileCheck %s --check-prefix=OVERALIGNED
; RUN: test ! -s %t/overaligned.o
; RUN: not llc -mtriple=mmix -filetype=asm %t/address-space.ll -o %t/address-space.s 2>&1 | FileCheck %s --check-prefix=ADDRESS-SPACE
; RUN: test ! -s %t/address-space.s
; RUN: not llc -mtriple=mmix -filetype=obj %t/address-space.ll -o %t/address-space.o 2>&1 | FileCheck %s --check-prefix=ADDRESS-SPACE
; RUN: test ! -s %t/address-space.o

; OVERALIGNED: MMIX does not support over-aligned formal arguments in function 'overaligned'
; ADDRESS-SPACE: MMIX does not support nonzero-address-space formal arguments in function 'nonzero_address_space'

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
