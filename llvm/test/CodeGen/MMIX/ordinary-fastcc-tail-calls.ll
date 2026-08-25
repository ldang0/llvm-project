; RUN: llc -mtriple=mmix -O0 -verify-machineinstrs %s -o - \
; RUN:   | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=mmix -O0 -verify-machineinstrs -filetype=obj %s -o %t.o
; RUN: llvm-objdump --no-print-imm-hex -d %t.o \
; RUN:   | FileCheck %s --check-prefix=OBJ
; RUN: opt -mtriple=mmix -passes=globalopt -S %s -o %t.opt.ll
; RUN: FileCheck %s --check-prefix=OPT < %t.opt.ll
; RUN: llc -mtriple=mmix -O2 -verify-machineinstrs %t.opt.ll -o - \
; RUN:   | FileCheck %s --check-prefix=OPT-ASM

define internal fastcc i64 @fast_target(i64 %value) noinline {
  ret i64 %value
}

; ASM-LABEL: fast_direct:
; ASM-NOT: PUSH
; ASM: JMP{{B?}} fast_target
; OBJ-LABEL: <fast_direct>:
; OBJ: JMP
define fastcc i64 @fast_direct(i64 %value) {
  %result = tail call fastcc i64 @fast_target(i64 %value)
  ret i64 %result
}

; ASM-LABEL: fast_indirect:
; ASM-NOT: PUSHGO
; ASM: GO r255
; OBJ-LABEL: <fast_indirect>:
; OBJ: GO r255
define fastcc i64 @fast_indirect(ptr %callee, i64 %value) {
  %result = tail call fastcc i64 %callee(i64 %value)
  ret i64 %result
}

; ASM-LABEL: fast_recursive:
; ASM: BZ
; ASM-NOT: PUSH
; ASM: JMPB fast_recursive
; OBJ-LABEL: <fast_recursive>:
; OBJ: JMPB
define fastcc i64 @fast_recursive(i64 %value) {
  %done = icmp eq i64 %value, 0
  br i1 %done, label %return, label %recurse

recurse:
  %next = sub i64 %value, 1
  %result = tail call fastcc i64 @fast_recursive(i64 %next)
  ret i64 %result

return:
  ret i64 %value
}

define internal fastcc i64 @fast_stack_target(
    i64, i64, i64, i64, i64, i64, i64, i64, i64,
    i64, i64, i64, i64, i64, i64, i64, i64 %last) noinline {
  ret i64 %last
}

; The caller's one incoming stack slot is reused after its value is loaded.
; ASM-LABEL: fast_stack_tail:
; ASM: LDO
; ASM: STOU
; ASM-NOT: PUSH
; ASM: JMP{{B?}} fast_stack_target
; OBJ-LABEL: <fast_stack_tail>:
; OBJ: JMP
define fastcc i64 @fast_stack_tail(
    i64 %a0, i64 %a1, i64 %a2, i64 %a3, i64 %a4, i64 %a5,
    i64 %a6, i64 %a7, i64 %a8, i64 %a9, i64 %a10, i64 %a11,
    i64 %a12, i64 %a13, i64 %a14, i64 %a15, i64 %a16) {
  %result = tail call fastcc i64 @fast_stack_target(
      i64 %a16, i64 %a15, i64 %a14, i64 %a13, i64 %a12, i64 %a11,
      i64 %a10, i64 %a9, i64 %a8, i64 %a7, i64 %a6, i64 %a5,
      i64 %a4, i64 %a3, i64 %a2, i64 %a1, i64 %a0)
  ret i64 %result
}

; Mismatched calling conventions are ordinary calls, not tail transfers.
; ASM-LABEL: c_to_fast_fallback:
; ASM: PUSHJ{{B?}}
; OBJ-LABEL: <c_to_fast_fallback>:
; OBJ: PUSHJ
define i64 @c_to_fast_fallback(i64 %value) {
  %result = tail call fastcc i64 @fast_target(i64 %value)
  ret i64 %result
}

define internal i64 @c_target(i64 %value) noinline {
  ret i64 %value
}

; ASM-LABEL: fast_to_c_fallback:
; ASM: PUSHJ{{B?}}
; OBJ-LABEL: <fast_to_c_fallback>:
; OBJ: PUSHJ
define fastcc i64 @fast_to_c_fallback(i64 %value) {
  %result = tail call i64 @c_target(i64 %value)
  ret i64 %result
}

define internal i64 @optimizer_tail_target(i64 %value) noinline {
  ret i64 %value
}

define internal i64 @optimizer_tail_caller(i64 %value) noinline {
  %result = tail call i64 @optimizer_tail_target(i64 %value)
  ret i64 %result
}

; GlobalOpt may give internal functions FastCC, while this externally visible
; C entry and its edge to the internal caller retain their source convention.
; OPT-LABEL: define internal fastcc i64 @optimizer_tail_target(
; OPT-LABEL: define internal fastcc i64 @optimizer_tail_caller(
; OPT: tail call fastcc i64 @optimizer_tail_target
; OPT-LABEL: define i64 @optimizer_entry(
; OPT: call fastcc i64 @optimizer_tail_caller
; OPT-ASM-LABEL: optimizer_tail_caller:
; OPT-ASM-NOT: PUSH
; OPT-ASM: JMP{{B?}} optimizer_tail_target
; OPT-ASM-LABEL: optimizer_entry:
; OPT-ASM: PUSHJ{{B?}}
define i64 @optimizer_entry(i64 %value) {
  %result = call i64 @optimizer_tail_caller(i64 %value)
  ret i64 %result
}
