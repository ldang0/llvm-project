; RUN: not --crash llc -mtriple=mmix %s -o /dev/null 2>&1 | FileCheck %s

; CHECK: LLVM ERROR: MMIX does not support thread-local storage

@tls = thread_local global i64 0

define i64 @load_tls() {
  %value = load i64, ptr @tls
  ret i64 %value
}
