; RUN: not --crash llc -mtriple=mmix %s -o /dev/null 2>&1 | FileCheck %s

; CHECK: LLVM ERROR: MMIX does not support exception handling

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
