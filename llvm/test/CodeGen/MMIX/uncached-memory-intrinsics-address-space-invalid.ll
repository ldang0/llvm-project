; RUN: not llvm-as -disable-output %s 2>&1 | FileCheck %s

; CHECK-DAG: intrinsic argument 0 type expected ptr, but got ptr addrspace(1)
; CHECK-DAG: declare i64 @llvm.mmix.ldunc(ptr addrspace(1))
; CHECK-DAG: declare void @llvm.mmix.stunc(ptr addrspace(1), i64)

declare i64 @llvm.mmix.ldunc(ptr addrspace(1))
declare void @llvm.mmix.stunc(ptr addrspace(1), i64)
