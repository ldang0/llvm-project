; RUN: split-file %s %t
; RUN: rm -f %t/formal.s %t/call.s
; RUN: not llc -mtriple=mmix -filetype=asm %t/formal.ll -o %t/formal.s 2>&1 | FileCheck %s --check-prefix=FORMAL
; RUN: test ! -s %t/formal.s
; RUN: not llc -mtriple=mmix -filetype=asm %t/call.ll -o %t/call.s 2>&1 | FileCheck %s --check-prefix=CALL
; RUN: test ! -s %t/call.s

; FORMAL: MMIX does not support aggregate or special formal arguments in function 'wide'
; CALL: MMIX does not support aggregate or special call arguments in function 'call_wide'

;--- formal.ll
target triple = "mmix"

%wide = type { i64, i8 }

define void @wide(%wide %value) {
  ret void
}

;--- call.ll
target triple = "mmix"

%wide = type { i64, i8 }

declare void @take_wide(%wide)

define void @call_wide(i64 %head, i8 %tail) {
  %v0 = insertvalue %wide poison, i64 %head, 0
  %v1 = insertvalue %wide %v0, i8 %tail, 1
  call void @take_wide(%wide %v1)
  ret void
}
