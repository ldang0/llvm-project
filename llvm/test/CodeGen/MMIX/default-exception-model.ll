; RUN: llc -mtriple=mmix -verify-machineinstrs %s -o %t.s
; RUN: FileCheck %s < %t.s
; RUN: llc -mtriple=mmix -exception-model=dwarf %s -o %t.explicit.s
; RUN: diff %t.s %t.explicit.s
; RUN: llc -mtriple=mmix -filetype=obj %s -o %t.o
; RUN: llvm-readobj -r %t.o | FileCheck %s --check-prefix=OBJ
; RUN: not llc -mtriple=mmix -exception-model=sjlj %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=INVALID

; CHECK-LABEL: invoke_callee:
; CHECK: .cfi_personality
; CHECK: _Unwind_Resume
; CHECK: .section .gcc_except_table
; OBJ: __gxx_personality_v0
; INVALID: MMIX supports only the DWARF exception model

declare void @callee()
declare i32 @__gxx_personality_v0(...)
declare fastcc void @fast_throwing_callee()

define fastcc void @fast_exception_path() personality ptr @__gxx_personality_v0 {
entry:
  invoke fastcc void @fast_throwing_callee()
      to label %return unwind label %cleanup
return:
  ret void
cleanup:
  %landing = landingpad { ptr, i32 } cleanup
  resume { ptr, i32 } %landing
}

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
