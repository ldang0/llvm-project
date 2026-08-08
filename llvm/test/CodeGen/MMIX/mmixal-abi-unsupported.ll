; RUN: split-file %s %t
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm -O0 \
; RUN:   --output-asm-variant=1 %t/aggregate-call.ll \
; RUN:   -o %t/aggregate-call.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=AGGREGATE
; RUN: test ! -s %t/aggregate-call.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm -O0 \
; RUN:   --output-asm-variant=1 %t/variadic-call.ll \
; RUN:   -o %t/variadic-call.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=VARIADIC
; RUN: test ! -s %t/variadic-call.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm -O0 \
; RUN:   --output-asm-variant=1 %t/wide-call.ll -o %t/wide-call.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=WIDE
; RUN: test ! -s %t/wide-call.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm -O0 \
; RUN:   --output-asm-variant=1 %t/alternate-cc.ll \
; RUN:   -o %t/alternate-cc.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=ALTERNATE-CC
; RUN: test ! -s %t/alternate-cc.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm -O0 \
; RUN:   --output-asm-variant=1 %t/musttail.ll -o %t/musttail.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=MUSTTAIL
; RUN: test ! -s %t/musttail.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm -O0 \
; RUN:   --output-asm-variant=1 %t/dynamic-alloca.ll \
; RUN:   -o %t/dynamic-alloca.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=DYNAMIC-ALLOCA
; RUN: test ! -s %t/dynamic-alloca.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm -O0 \
; RUN:   --output-asm-variant=1 %t/stack-realignment.ll \
; RUN:   -o %t/stack-realignment.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=STACK-REALIGNMENT
; RUN: test ! -s %t/stack-realignment.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm -O0 \
; RUN:   --output-asm-variant=1 %t/coroutine.ll \
; RUN:   -o %t/coroutine.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=COROUTINE
; RUN: test ! -s %t/coroutine.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm -O0 \
; RUN:   --output-asm-variant=1 %t/nonlocal-stack.ll \
; RUN:   -o %t/nonlocal-stack.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=NONLOCAL-STACK
; RUN: test ! -s %t/nonlocal-stack.mms

; AGGREGATE: LLVM ERROR: MMIX does not support aggregate or special call arguments in function 'aggregate_owner'
; VARIADIC: LLVM ERROR: MMIX does not support variadic calls in function 'variadic_owner'
; WIDE: LLVM ERROR: MMIX does not support aggregate or special call arguments in function 'wide_owner'
; ALTERNATE-CC: LLVM ERROR: MMIX supports only the C calling convention in function 'alternate_cc_owner'
; MUSTTAIL: LLVM ERROR: MMIX does not support required tail calls in function 'musttail_owner'
; DYNAMIC-ALLOCA: LLVM ERROR: MMIX does not support dynamic stack allocation in function 'dynamic_owner'
; STACK-REALIGNMENT: LLVM ERROR: MMIX does not support stack realignment in function 'realignment_owner'
; COROUTINE: LLVM ERROR: MMIX does not support coroutines in function 'coroutine_owner'
; NONLOCAL-STACK: LLVM ERROR: MMIX does not support nonlocal stack state in function 'stack_owner'

;--- aggregate-call.ll
target triple = "mmix-unknown-elf"

%pair = type { i64, i64 }

define void @aggregate_owner(ptr %value) {
  call void @aggregate_target(ptr byval(%pair) %value)
  ret void
}

define void @Main() {
  br label %loop
loop:
  br label %loop
}

define void @aggregate_target(ptr byval(%pair) %value) {
  ret void
}

;--- variadic-call.ll
target triple = "mmix-unknown-elf"

define void @variadic_owner() {
  call void (i64, ...) @variadic_target(i64 1, i64 2)
  ret void
}

define void @Main() {
  br label %loop
loop:
  br label %loop
}

define void @variadic_target(i64 %fixed, ...) {
  ret void
}

;--- wide-call.ll
target triple = "mmix-unknown-elf"

define void @wide_owner() {
  %wide = zext i64 1 to i128
  call void @wide_target(i128 %wide)
  ret void
}

define void @Main() {
  br label %loop
loop:
  br label %loop
}

define void @wide_target(i128 %value) {
  ret void
}

;--- alternate-cc.ll
target triple = "mmix-unknown-elf"

define void @alternate_cc_owner() {
  call fastcc void @alternate_cc_target()
  ret void
}

define void @Main() {
  br label %loop
loop:
  br label %loop
}

define fastcc void @alternate_cc_target() {
  ret void
}

;--- musttail.ll
target triple = "mmix-unknown-elf"

define i64 @musttail_owner(i64 %value) {
  %result = musttail call i64 @musttail_target(i64 %value)
  ret i64 %result
}

define void @Main() {
  br label %loop
loop:
  br label %loop
}

define i64 @musttail_target(i64 %value) {
  ret i64 %value
}

;--- dynamic-alloca.ll
target triple = "mmix-unknown-elf"

define void @dynamic_owner(i64 %count) {
  %storage = alloca i64, i64 %count, align 8
  store volatile i64 0, ptr %storage
  ret void
}

define void @Main() {
  br label %loop
loop:
  br label %loop
}

;--- stack-realignment.ll
target triple = "mmix-unknown-elf"

define void @realignment_owner() {
  %storage = alloca i64, align 16
  store volatile i64 0, ptr %storage, align 16
  ret void
}

define void @Main() {
  br label %loop
loop:
  br label %loop
}

;--- coroutine.ll
target triple = "mmix-unknown-elf"

define void @coroutine_owner() presplitcoroutine {
  ret void
}

define void @Main() {
  br label %loop
loop:
  br label %loop
}

;--- nonlocal-stack.ll
target triple = "mmix-unknown-elf"

declare ptr @llvm.stacksave()

define void @stack_owner() {
  %stack = call ptr @llvm.stacksave()
  ret void
}

define void @Main() {
  br label %loop
loop:
  br label %loop
}
