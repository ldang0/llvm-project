; RUN: not llc -mtriple=mmix %s -o /dev/null 2>&1 | FileCheck %s

target triple = "mmix"

; CHECK: error: unsupported atomicrmw add: target supports atomics up to 8 bytes, but this atomic accesses 16 bytes
define i128 @wide_atomic_rmw(ptr %p, i128 %value) {
  %old = atomicrmw add ptr %p, i128 %value monotonic
  ret i128 %old
}
