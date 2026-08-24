; RUN: llc -mtriple=mmix-unknown-elf -filetype=asm %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=mmix-unknown-elf -filetype=obj %s -o %t.o
; RUN: llvm-objdump -d %t.o | FileCheck %s --check-prefix=OBJ

; FastCC initially uses the C convention's physical assignments. These focused
; checks establish explicit scalar definitions and direct, indirect, recursive,
; register, stack, and result paths before TTI may generate FastCC.

; ASM-LABEL: fast_identity:
; ASM:       POP 0, 0
define internal fastcc i64 @fast_identity(i64 %value) noinline {
  ret i64 %value
}

; ASM-LABEL: direct_caller:
; ASM:       PUSHJB r31, fast_identity
; OBJ-LABEL: <direct_caller>:
; OBJ:       PUSHJB r31,
define i64 @direct_caller(i64 %value) {
  %result = call fastcc i64 @fast_identity(i64 %value)
  ret i64 %result
}

; ASM-LABEL: indirect_caller:
; ASM:       PUSHGO r31, {{r[0-9]+}}, 0
; OBJ-LABEL: <indirect_caller>:
; OBJ:       PUSHGO r31,
define i64 @indirect_caller(ptr %callee, i64 %value) {
  %result = call fastcc i64 %callee(i64 %value)
  ret i64 %result
}

; ASM-LABEL: recursive_fast:
; ASM:       PUSHJB r31, recursive_fast
define internal fastcc i64 @recursive_fast(i64 %value) noinline {
  %done = icmp eq i64 %value, 0
  br i1 %done, label %return, label %recurse

recurse:
  %next = sub i64 %value, 1
  %nested = call fastcc i64 @recursive_fast(i64 %next)
  ret i64 %nested

return:
  ret i64 %value
}

; The seventeenth argument uses the first outgoing and incoming stack slot.
; ASM-LABEL: fast_stack_argument:
; ASM:       LDOU r231, r254, 0
define internal fastcc i64 @fast_stack_argument(
    i64, i64, i64, i64, i64, i64, i64, i64,
    i64, i64, i64, i64, i64, i64, i64, i64, i64 %last) noinline {
  ret i64 %last
}

; ASM-LABEL: stack_caller:
; ASM:       OR [[OUTSP:r[0-9]+]], r254, 0
; ASM:       STOU r231, [[OUTSP]], 0
; ASM:       PUSHJB r31, fast_stack_argument
define i64 @stack_caller(i64 %value) {
  %result = call fastcc i64 @fast_stack_argument(
      i64 0, i64 1, i64 2, i64 3, i64 4, i64 5, i64 6, i64 7,
      i64 8, i64 9, i64 10, i64 11, i64 12, i64 13, i64 14, i64 15,
      i64 %value)
  ret i64 %result
}
