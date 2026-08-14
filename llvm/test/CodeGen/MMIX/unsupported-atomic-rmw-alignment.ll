; RUN: not llc -mtriple=mmix %s -o /dev/null 2>&1 | FileCheck %s

target triple = "mmix"

; CHECK: error: unsupported atomicrmw add: instruction alignment 2 is smaller than the required 4-byte alignment for this atomic operation
define i32 @unaligned_atomic_rmw(ptr %p, i32 %value) {
  %old = atomicrmw add ptr %p, i32 %value monotonic, align 2
  ret i32 %old
}
