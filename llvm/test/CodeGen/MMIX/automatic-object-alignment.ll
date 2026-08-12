; RUN: llc -mtriple=mmix -verify-machineinstrs -stop-after=prolog-epilog %s -o - \
; RUN:   | FileCheck %s

target triple = "mmix"

%packed = type <{ i8, i16 }>

; MMIX follows the GNU ABI by placing fixed automatic objects at alignments
; of at least four bytes without changing their LLVM IR alignment.
; CHECK-LABEL: name: fixed_automatic_objects
; CHECK: stack:
; CHECK: name: byte,{{.*}}size: 1, alignment: 4
; CHECK: name: half,{{.*}}size: 2, alignment: 4
; CHECK: name: bytes,{{.*}}size: 3, alignment: 4
; CHECK: name: packed,{{.*}}size: 3, alignment: 4
; CHECK: name: word,{{.*}}size: 4, alignment: 4
; CHECK: name: octa,{{.*}}size: 8, alignment: 8
; CHECK: name: aligned_eight,{{.*}}size: 1,
; CHECK-NEXT: alignment: 8,
define void @fixed_automatic_objects() {
entry:
  %byte = alloca i8, align 1
  %half = alloca i16, align 2
  %bytes = alloca [3 x i8], align 1
  %packed = alloca %packed, align 1
  %word = alloca i32, align 4
  %octa = alloca i64, align 8
  %aligned_eight = alloca i8, align 8
  store volatile i8 1, ptr %byte, align 1
  store volatile i16 2, ptr %half, align 2
  store volatile i8 1, ptr %bytes, align 1
  store volatile %packed <{ i8 1, i16 2 }>, ptr %packed, align 1
  store volatile i32 3, ptr %word, align 4
  store volatile i64 4, ptr %octa, align 8
  store volatile i8 1, ptr %aligned_eight, align 8
  ret void
}
