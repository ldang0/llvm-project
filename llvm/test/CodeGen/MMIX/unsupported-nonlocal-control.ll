; RUN: split-file %s %t
; RUN: not llc -mtriple=mmix -filetype=asm %t/stack-state.ll \
; RUN:   -o %t/stack-state.s 2>&1 | FileCheck %s --check-prefix=STACK-STATE
; RUN: test ! -s %t/stack-state.s
; RUN: not llc -mtriple=mmix -filetype=obj %t/stack-state.ll \
; RUN:   -o %t/stack-state.o 2>&1 | FileCheck %s --check-prefix=STACK-STATE
; RUN: test ! -s %t/stack-state.o
; RUN: not llc -mtriple=mmix -filetype=asm %t/sjlj.ll \
; RUN:   -o %t/sjlj.s 2>&1 | FileCheck %s --check-prefix=SJLJ
; RUN: test ! -s %t/sjlj.s
; RUN: not llc -mtriple=mmix -filetype=obj %t/sjlj.ll \
; RUN:   -o %t/sjlj.o 2>&1 | FileCheck %s --check-prefix=SJLJ
; RUN: test ! -s %t/sjlj.o

; STACK-STATE: LLVM ERROR: MMIX does not support nonlocal stack state in function 'restore_stack'
; SJLJ: LLVM ERROR: MMIX does not support nonlocal control transfer in function 'save_context'

;--- stack-state.ll
declare ptr @llvm.stacksave()
declare void @llvm.stackrestore(ptr)

define void @restore_stack() {
  %stack = call ptr @llvm.stacksave()
  call void @llvm.stackrestore(ptr %stack)
  ret void
}

;--- sjlj.ll
declare i32 @llvm.eh.sjlj.setjmp(ptr)

define i32 @save_context(ptr %buffer) {
  %result = call i32 @llvm.eh.sjlj.setjmp(ptr %buffer)
  ret i32 %result
}
