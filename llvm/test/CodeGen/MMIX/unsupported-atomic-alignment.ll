; RUN: not llc -mtriple=mmix %s -o /dev/null 2>&1 | FileCheck %s

target triple = "mmix"

; CHECK: error: unsupported cmpxchg: instruction alignment 1 is smaller than the required 8-byte alignment for this atomic operation
define void @unaligned_cmpxchg(ptr %p) {
  %pair = cmpxchg ptr %p, i64 0, i64 1 monotonic monotonic, align 1
  ret void
}
