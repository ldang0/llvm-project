; RUN: split-file %s %t
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm -O0 \
; RUN:   --output-asm-variant=1 %t/external-function.ll \
; RUN:   -o %t/external-function.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=EXTERNAL-FUNCTION
; RUN: test ! -s %t/external-function.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm -O0 \
; RUN:   --output-asm-variant=1 %t/external-data.ll \
; RUN:   -o %t/external-data.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=EXTERNAL-DATA
; RUN: test ! -s %t/external-data.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm -O0 \
; RUN:   --output-asm-variant=1 %t/runtime-helper.ll \
; RUN:   -o %t/runtime-helper.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=RUNTIME-HELPER
; RUN: test ! -s %t/runtime-helper.mms
; RUN: llc -mtriple=mmix-unknown-elf -filetype=asm -O0 \
; RUN:   --output-asm-variant=1 %t/resolved-helper.ll -o - \
; RUN:   | FileCheck %s --check-prefix=RESOLVED-HELPER

; EXTERNAL-FUNCTION: LLVM ERROR: MMIXAL output variant 1 cannot resolve referenced symbol 'external_function'
; EXTERNAL-FUNCTION-NOT: __LLVM_
; EXTERNAL-DATA: LLVM ERROR: MMIXAL output variant 1 cannot resolve referenced symbol 'external_data'
; EXTERNAL-DATA-NOT: __LLVM_
; RUNTIME-HELPER: LLVM ERROR: MMIXAL output variant 1 cannot resolve referenced symbol 'memcpy'
; RUNTIME-HELPER-NOT: __LLVM_
; RESOLVED-HELPER: memcpy IS @
; RESOLVED-HELPER: SETH ${{[0-9]+}}, memcpy>>48&65535
; RESOLVED-HELPER: PUSHGO $31, ${{[0-9]+}}, 0

;--- external-function.ll
target triple = "mmix-unknown-elf"

declare void @external_function()

define void @Main() {
entry:
  call void @external_function()
  br label %loop

loop:
  br label %loop
}

;--- external-data.ll
target triple = "mmix-unknown-elf"

@external_data = external global i64

define void @Main() {
entry:
  %value = load volatile i64, ptr @external_data
  br label %loop

loop:
  br label %loop
}

;--- runtime-helper.ll
target triple = "mmix-unknown-elf"

declare ptr @memcpy(ptr, ptr, i64)

define void @Main() {
entry:
  br label %loop

loop:
  br label %loop
}

define void @copy(ptr %destination, ptr %source, i64 %size) {
entry:
  %result = call ptr @memcpy(ptr %destination, ptr %source, i64 %size)
  ret void
}

;--- resolved-helper.ll
target triple = "mmix-unknown-elf"

declare void @unused()
declare i64 @llvm.ctpop.i64(i64)

define void @Main() {
entry:
  %destination = inttoptr i64 256 to ptr
  %source = inttoptr i64 512 to ptr
  %result = call ptr @memcpy(ptr %destination, ptr %source, i64 8)
  br label %loop

loop:
  br label %loop
}

define ptr @memcpy(ptr %destination, ptr %source, i64 %size) {
entry:
  ret ptr %destination
}
