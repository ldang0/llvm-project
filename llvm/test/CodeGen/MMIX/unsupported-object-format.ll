; LLVM intentionally has no MMO code-generation file type.  Keep that format
; outside the MMIX target before an output file can be created.
; RUN: not llc -mtriple=mmix-unknown-unknown -filetype=mmo %s \
; RUN:   -o %t.mmo 2>&1 | FileCheck %s
; RUN: test ! -e %t.mmo

; CHECK: llc: for the --filetype option: Cannot find option named 'mmo'!

define void @unsupported_object_format() {
  ret void
}
