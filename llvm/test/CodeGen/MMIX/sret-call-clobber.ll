; RUN: llc -mtriple=mmix -O2 -verify-machineinstrs -stop-after=machine-sink %s -o - | FileCheck %s
; RUN: llc -mtriple=mmix -O2 -verify-machineinstrs %s -o /dev/null

%result = type { i64 }

; The reserved incoming sret register is not invariant across a call. Its
; copy must stay in the entry block, even when only the successor uses it.
; CHECK-LABEL: name: direct
; CHECK: bb.0.entry:
; CHECK: [[RESULT:%[0-9]+]]:{{[a-z0-9]+}} = COPY $r251
; CHECK: DIRECT_CALL_STATE {{.*}}implicit-def {{(dead )?}}$r251
; CHECK: bb.1.done:
; CHECK-NOT: COPY $r251
; CHECK: STOUI {{.*}}[[RESULT]]
define void @direct(ptr sret(%result) %result) personality ptr @__gxx_personality_v0 {
entry:
  %value = invoke i64 @callee() to label %done unwind label %unwind
done:
  store i64 %value, ptr %result
  ret void
unwind:
  %landing = landingpad { ptr, i32 } cleanup
  resume { ptr, i32 } %landing
}

; CHECK-LABEL: name: indirect
; CHECK: bb.0.entry:
; CHECK: [[RESULT:%[0-9]+]]:{{[a-z0-9]+}} = COPY $r251
; CHECK: CALL_STATE {{.*}}implicit-def {{(dead )?}}$r251
; CHECK: bb.1.done:
; CHECK-NOT: COPY $r251
; CHECK: STOUI {{.*}}[[RESULT]]
define void @indirect(ptr sret(%result) %result, ptr %callee) personality ptr @__gxx_personality_v0 {
entry:
  %value = invoke i64 %callee() to label %done unwind label %unwind
done:
  store i64 %value, ptr %result
  ret void
unwind:
  %landing = landingpad { ptr, i32 } cleanup
  resume { ptr, i32 } %landing
}

declare i64 @callee()
declare i32 @__gxx_personality_v0(...)
