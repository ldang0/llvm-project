; RUN: not --crash llc -mtriple=mmix %s -o /dev/null 2>&1 | FileCheck %s

; CHECK: LLVM ERROR: MMIX does not support nonzero address spaces

@value = addrspace(1) global i64 0

define i64 @load_nonzero_address_space() {
  %value = load i64, ptr addrspace(1) @value
  ret i64 %value
}
