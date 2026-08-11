; RUN: split-file %s %t
; RUN: not llc -mtriple=mmix -filetype=asm %t/formal.ll -o %t/formal.s 2>&1 | FileCheck %s --check-prefix=FORMAL
; RUN: test ! -s %t/formal.s
; RUN: not llc -mtriple=mmix -filetype=obj %t/formal.ll -o %t/formal.o 2>&1 | FileCheck %s --check-prefix=FORMAL
; RUN: test ! -s %t/formal.o
; RUN: not llc -mtriple=mmix -filetype=asm %t/call.ll -o %t/call.s 2>&1 | FileCheck %s --check-prefix=CALL
; RUN: test ! -s %t/call.s
; RUN: not llc -mtriple=mmix -filetype=obj %t/call.ll -o %t/call.o 2>&1 | FileCheck %s --check-prefix=CALL
; RUN: test ! -s %t/call.o
; RUN: not llc -mtriple=mmix -filetype=asm %t/result.ll -o %t/result.s 2>&1 | FileCheck %s --check-prefix=RESULT
; RUN: test ! -s %t/result.s
; RUN: not llc -mtriple=mmix -filetype=obj %t/result.ll -o %t/result.o 2>&1 | FileCheck %s --check-prefix=RESULT
; RUN: test ! -s %t/result.o

; FORMAL: MMIX does not support ABI type 'half' for formal arguments in function 'half_formal'
; CALL: MMIX does not support ABI type 'half' for call arguments in function 'call_half'
; RESULT: MMIX does not support ABI type 'half' for function results in function 'half_result'

;--- formal.ll
target triple = "mmix"

define void @half_formal(half %value) {
  ret void
}

;--- call.ll
target triple = "mmix"

declare void @take_half(half)

define void @call_half(i16 %bits) {
  %value = bitcast i16 %bits to half
  call void @take_half(half %value)
  ret void
}

;--- result.ll
target triple = "mmix"

define half @half_result() {
  ret half 0xH0000
}
