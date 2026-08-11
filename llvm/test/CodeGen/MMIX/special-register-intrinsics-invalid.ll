; RUN: not llc -mtriple=mmix %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=COMMON
; RUN: not llc -mtriple=mmix -mattr=-system,-virtual-memory %s -o /dev/null 2>&1 | FileCheck %s --check-prefixes=COMMON,SYSTEM,VM

; COMMON: error: llvm.mmix.get: selector must be in the range [0, 31]
; COMMON: error: llvm.mmix.get: selector must be in the range [0, 31]
; COMMON: error: llvm.mmix.put: selector must be in the range [0, 31]
; COMMON-DAG: error: llvm.mmix.put: register 'rN' is architecturally read-only
; COMMON-DAG: error: llvm.mmix.put: register 'rO' is architecturally read-only
; COMMON-DAG: error: llvm.mmix.put: register 'rS' is architecturally read-only
; COMMON-DAG: error: llvm.mmix.put: register 'rJ' is reserved by the MMIX C ABI
; COMMON-DAG: error: llvm.mmix.put: register 'rG' is reserved by the MMIX C ABI
; COMMON-DAG: error: llvm.mmix.put: register 'rL' is reserved by the MMIX C ABI
; COMMON: error: llvm.mmix.put: register 'rA' requires explicit floating-environment modeling
; SYSTEM: error: llvm.mmix.put: register 'rC' requires the system target feature
; VM: error: llvm.mmix.put: register 'rV' requires the virtual-memory target feature

declare i64 @llvm.mmix.get(i32 immarg)
declare void @llvm.mmix.put(i32 immarg, i64)

define i64 @get_negative() {
  %value = call i64 @llvm.mmix.get(i32 -1)
  ret i64 %value
}

define i64 @get_large() {
  %value = call i64 @llvm.mmix.get(i32 32)
  ret i64 %value
}

define void @put_large() {
  call void @llvm.mmix.put(i32 32, i64 0)
  ret void
}

define void @put_read_only() {
  call void @llvm.mmix.put(i32 9, i64 0)
  call void @llvm.mmix.put(i32 10, i64 0)
  call void @llvm.mmix.put(i32 11, i64 0)
  ret void
}

define void @put_abi() {
  call void @llvm.mmix.put(i32 4, i64 0)
  call void @llvm.mmix.put(i32 19, i64 0)
  call void @llvm.mmix.put(i32 20, i64 0)
  ret void
}

define void @put_floating_environment() {
  call void @llvm.mmix.put(i32 21, i64 0)
  ret void
}

define void @put_system() {
  call void @llvm.mmix.put(i32 8, i64 0)
  ret void
}

define void @put_virtual_memory() {
  call void @llvm.mmix.put(i32 18, i64 0)
  ret void
}
