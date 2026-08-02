; RUN: not --crash llc -mtriple=mmix -stop-after=mmix-isel \
; RUN:   -o /dev/null %s 2>&1 | FileCheck %s

; CHECK: MMIX SelectionDAG operation is not implemented by this lowering stage: GlobalAddress

@value = global i64 0

define void @unsupported_global_address() {
entry:
  store volatile i64 1, ptr @value
  unreachable
}
