; RUN: split-file %s %t
; RUN: not llc -mtriple=mmix -filetype=asm %t/variable.ll -o %t/variable.s 2>&1 | FileCheck %s --check-prefix=VARIABLE
; RUN: test ! -s %t/variable.s
; RUN: not llc -mtriple=mmix -filetype=obj %t/variable.ll -o %t/variable.o 2>&1 | FileCheck %s --check-prefix=VARIABLE
; RUN: test ! -s %t/variable.o
; RUN: not llc -mtriple=mmix -filetype=asm %t/formal-flag.ll -o %t/formal-flag.s 2>&1 | FileCheck %s --check-prefix=FORMAL-FLAG
; RUN: test ! -s %t/formal-flag.s
; RUN: not llc -mtriple=mmix -filetype=obj %t/formal-flag.ll -o %t/formal-flag.o 2>&1 | FileCheck %s --check-prefix=FORMAL-FLAG
; RUN: test ! -s %t/formal-flag.o
; RUN: not llc -mtriple=mmix -filetype=asm %t/call-flag.ll -o %t/call-flag.s 2>&1 | FileCheck %s --check-prefix=CALL-FLAG
; RUN: test ! -s %t/call-flag.s
; RUN: not llc -mtriple=mmix -filetype=obj %t/call-flag.ll -o %t/call-flag.o 2>&1 | FileCheck %s --check-prefix=CALL-FLAG
; RUN: test ! -s %t/call-flag.o

; VARIABLE: MMIX does not support variable-size formal arguments in function 'variable_size'
; FORMAL-FLAG: MMIX does not support formal arguments with unsupported ABI flags in function 'inreg_formal'
; CALL-FLAG: MMIX does not support call arguments with unsupported ABI flags in function 'call_inreg'

;--- variable.ll
target triple = "mmix"

%scalable = type { <vscale x 2 x i64> }

define void @variable_size(%scalable %value) {
  ret void
}

;--- formal-flag.ll
target triple = "mmix"

define void @inreg_formal(i64 inreg %value) {
  ret void
}

;--- call-flag.ll
target triple = "mmix"

declare void @take_inreg(i64 inreg)

define void @call_inreg(i64 %value) {
  call void @take_inreg(i64 inreg %value)
  ret void
}
