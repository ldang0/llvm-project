; RUN: split-file %s %t
; RUN: not llc -mtriple=mmix %t/stack-state.ll \
; RUN:   -o /dev/null 2>&1 | FileCheck %s --check-prefix=STACK-STATE
; RUN: not llc -mtriple=mmix %t/sjlj.ll \
; RUN:   -o /dev/null 2>&1 | FileCheck %s --check-prefix=SJLJ

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
