; RUN: split-file %s %t
; RUN: not llc -mtriple=mmix -filetype=asm %t/callee.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=CALLEE
; RUN: not llc -mtriple=mmix -filetype=asm %t/caller.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=CALLER

; CALLEE: MMIX does not support aggregate function results in function 'return_wide'
; CALLER: MMIX does not support aggregate call results in function 'call_wide'

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
