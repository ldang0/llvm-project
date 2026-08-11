; RUN: split-file %s %t
; RUN: not llc -mtriple=mmix -filetype=asm %t/callee.ll -o %t/callee.s 2>&1 | FileCheck %s --check-prefix=CALLEE
; RUN: test ! -s %t/callee.s
; RUN: not llc -mtriple=mmix -filetype=obj %t/callee.ll -o %t/callee.o 2>&1 | FileCheck %s --check-prefix=CALLEE
; RUN: test ! -s %t/callee.o
; RUN: not llc -mtriple=mmix -filetype=asm %t/caller.ll -o %t/caller.s 2>&1 | FileCheck %s --check-prefix=CALLER
; RUN: test ! -s %t/caller.s
; RUN: not llc -mtriple=mmix -filetype=obj %t/caller.ll -o %t/caller.o 2>&1 | FileCheck %s --check-prefix=CALLER
; RUN: test ! -s %t/caller.o

; CALLEE: MMIX does not support multi-register function results in function 'return_wide'
; CALLER: MMIX does not support multi-register call results in function 'call_wide'

;--- callee.ll
target triple = "mmix"

%wide = type { i64, i8 }

define %wide @return_wide() {
  ret %wide zeroinitializer
}

;--- caller.ll
target triple = "mmix"

%wide = type { i64, i8 }

declare %wide @make_wide()

define i64 @call_wide() {
  %value = call %wide @make_wide()
  %field = extractvalue %wide %value, 0
  ret i64 %field
}
