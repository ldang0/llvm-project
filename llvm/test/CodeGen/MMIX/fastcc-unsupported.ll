; RUN: split-file %s %t
; RUN: not llc -mtriple=mmix -filetype=asm %t/vararg.ll \
; RUN:   -o %t/vararg.s 2>&1 | FileCheck %s --check-prefix=VARARG
; RUN: test ! -s %t/vararg.s
; RUN: not llc -mtriple=mmix -filetype=obj %t/vararg.ll \
; RUN:   -o %t/vararg.o 2>&1 | FileCheck %s --check-prefix=VARARG
; RUN: test ! -s %t/vararg.o
; RUN: not llc -mtriple=mmix -filetype=asm %t/runtime-cc.ll \
; RUN:   -o %t/runtime-cc.s 2>&1 | FileCheck %s --check-prefix=RUNTIME-CC
; RUN: test ! -s %t/runtime-cc.s
; RUN: not llc -mtriple=mmix -filetype=obj %t/runtime-cc.ll \
; RUN:   -o %t/runtime-cc.o 2>&1 | FileCheck %s --check-prefix=RUNTIME-CC
; RUN: test ! -s %t/runtime-cc.o
; RUN: not llc -mtriple=mmix -filetype=asm %t/coroutine.ll \
; RUN:   -o %t/coroutine.s 2>&1 | FileCheck %s --check-prefix=COROUTINE
; RUN: test ! -s %t/coroutine.s
; RUN: not llc -mtriple=mmix -filetype=obj %t/coroutine.ll \
; RUN:   -o %t/coroutine.o 2>&1 | FileCheck %s --check-prefix=COROUTINE
; RUN: test ! -s %t/coroutine.o

; VARARG: Calling convention does not support varargs or perfect forwarding!
; VARARG: input module cannot be verified
; RUNTIME-CC: LLVM ERROR: MMIX supports only C and Fast calling conventions in function 'runtime_call'
; COROUTINE: LLVM ERROR: MMIX does not support coroutines in function 'fast_coroutine'

;--- vararg.ll
target triple = "mmix-unknown-elf"

declare fastcc i64 @variadic_fast(i64, ...)

define i64 @call_variadic_fast(i64 %value) {
  %result = call fastcc i64 (i64, ...) @variadic_fast(i64 %value, i64 1)
  ret i64 %result
}

;--- runtime-cc.ll
target triple = "mmix-unknown-elf"

declare preserve_mostcc i64 @runtime_callee(i64)

define i64 @runtime_call(i64 %value) {
  %result = call preserve_mostcc i64 @runtime_callee(i64 %value)
  ret i64 %result
}

;--- coroutine.ll
target triple = "mmix-unknown-elf"

define fastcc void @fast_coroutine() presplitcoroutine {
  ret void
}
