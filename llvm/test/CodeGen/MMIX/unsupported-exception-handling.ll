; RUN: not llc -mtriple=mmix -filetype=asm %s -o %t.s 2>&1 | FileCheck %s
; RUN: test ! -s %t.s
; RUN: not llc -mtriple=mmix -filetype=obj %s -o %t.o 2>&1 | FileCheck %s
; RUN: test ! -s %t.o

; CHECK: LLVM ERROR: MMIX exception handling requires the explicit DWARF model in function 'invoke_callee'

declare void @callee()
declare i32 @__gxx_personality_v0(...)

define void @invoke_callee() personality ptr @__gxx_personality_v0 {
entry:
  invoke void @callee() to label %return unwind label %cleanup

return:
  ret void

cleanup:
  %landing = landingpad { ptr, i32 }
      cleanup
  resume { ptr, i32 } %landing
}
