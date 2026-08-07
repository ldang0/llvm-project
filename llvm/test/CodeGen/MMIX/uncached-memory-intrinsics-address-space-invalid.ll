; RUN: not llvm-as -disable-output %s 2>&1 | FileCheck %s

; CHECK: intrinsic argument 0 type expected ptr, but got ptr addrspace(1)
; CHECK-NEXT: declare i64 @llvm.mmix.ldunc(ptr addrspace(1))

declare i64 @llvm.mmix.ldunc(ptr addrspace(1))
