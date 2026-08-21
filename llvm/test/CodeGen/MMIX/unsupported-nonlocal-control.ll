; RUN: split-file %s %t
; RUN: not llc -mtriple=mmix -filetype=asm %t/sjlj.ll \
; RUN:   -o %t/sjlj.s 2>&1 | FileCheck %s --check-prefix=SJLJ
; RUN: test ! -s %t/sjlj.s
; RUN: not llc -mtriple=mmix -filetype=obj %t/sjlj.ll \
; RUN:   -o %t/sjlj.o 2>&1 | FileCheck %s --check-prefix=SJLJ
; RUN: test ! -s %t/sjlj.o

; SJLJ: LLVM ERROR: MMIX does not support nonlocal control transfer in function 'save_context'

;--- sjlj.ll
declare i32 @llvm.eh.sjlj.setjmp(ptr)

define i32 @save_context(ptr %buffer) {
  %result = call i32 @llvm.eh.sjlj.setjmp(ptr %buffer)
  ret i32 %result
}
