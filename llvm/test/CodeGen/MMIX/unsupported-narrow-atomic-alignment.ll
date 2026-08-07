; RUN: not llc -mtriple=mmix %s -o /dev/null 2>&1 | FileCheck %s

target triple = "mmix"

; CHECK: error: unsupported atomic load: instruction alignment 2 is smaller than the required 4-byte alignment for this atomic operation
define i32 @unaligned_atomic_load(ptr %p) {
  %value = load atomic i32, ptr %p monotonic, align 2
  ret i32 %value
}
