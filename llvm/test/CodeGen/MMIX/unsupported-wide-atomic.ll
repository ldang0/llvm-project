; RUN: not llc -mtriple=mmix %s -o /dev/null 2>&1 | FileCheck %s

target triple = "mmix"

; CHECK: error: unsupported cmpxchg: target supports atomics up to 8 bytes, but this atomic accesses 16 bytes
define void @wide_cmpxchg(ptr %p) {
  %pair = cmpxchg ptr %p, i128 0, i128 1 monotonic monotonic
  ret void
}
