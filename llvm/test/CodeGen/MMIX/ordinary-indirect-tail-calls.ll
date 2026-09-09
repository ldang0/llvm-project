; RUN: opt -passes='default<O2>' -S %s -o - \
; RUN:   | FileCheck %s --check-prefix=OPT
; RUN: llc -mtriple=mmix -O2 -verify-machineinstrs %s -o - \
; RUN:   | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=mmix -O2 -verify-machineinstrs -filetype=obj %s -o %t.o
; RUN: llvm-readobj --relocations %t.o \
; RUN:   | FileCheck %s --check-prefix=RELOC
; RUN: llvm-objdump --no-print-imm-hex -dr %t.o \
; RUN:   | FileCheck %s --check-prefix=OBJ

; OPT-LABEL: define i64 @indirect_tail(
; OPT: tail call i64 %callee(i64 %value)
define i64 @indirect_tail(ptr %callee, i64 %value) nounwind {
  %result = tail call i64 %callee(i64 %value)
  ret i64 %result
}

; A self pointer remains live as both callee and an outgoing argument.
; ASM-LABEL: function_pointer_recursion:
; ASM-NOT: PUSHGO
; ASM: GO r255
define i64 @function_pointer_recursion(ptr %self, i64 %value) nounwind {
  %result = tail call i64 %self(ptr %self, i64 %value)
  ret i64 %result
}

; The callee starts in the first argument register while the outgoing value
; replaces it. Argument preparation must retain the callee before that copy.
; ASM-LABEL: callee_argument_overlap:
; ASM-NOT: PUSHGO
; ASM: OR [[CALLEE:r[0-9]+]], r231, 0
; ASM: OR r231, r232, 0
; ASM: GO r255, [[CALLEE]], 0
define i64 @callee_argument_overlap(ptr %callee, i64 %value) nounwind {
  %result = tail call i64 %callee(i64 %value, ptr %callee)
  ret i64 %result
}

%pair = type { i64, i64 }

; ASM-LABEL: forwarded_sret_indirect:
; ASM-NOT: PUSHGO
; ASM: GO r255
define void @forwarded_sret_indirect(ptr %callee, ptr sret(%pair) %result,
                                     i64 %value) nounwind {
  tail call void %callee(ptr sret(%pair) %result, i64 %value)
  ret void
}

; Incoming stack values are loaded before the reused outgoing slots are
; overwritten and before the terminal transfer.
; ASM-LABEL: stack_value_overlap:
; ASM: LDO
; ASM: STOU
; ASM-NOT: PUSHGO
; ASM: GO r255
define i64 @stack_value_overlap(
    ptr %callee, i64 %a0, i64 %a1, i64 %a2, i64 %a3, i64 %a4, i64 %a5,
    i64 %a6, i64 %a7, i64 %a8, i64 %a9, i64 %a10, i64 %a11, i64 %a12,
    i64 %a13, i64 %a14, i64 %a15, i64 %a16, i64 %a17) nounwind {
  %result = tail call i64 %callee(
      i64 %a17, i64 %a16, i64 %a15, i64 %a14, i64 %a13, i64 %a12,
      i64 %a11, i64 %a10, i64 %a9, i64 %a8, i64 %a7, i64 %a6,
      i64 %a5, i64 %a4, i64 %a3, i64 %a2, i64 %a1, i64 %a0)
  ret i64 %result
}

; This caller has no reusable incoming stack area for the seventeenth value.
; ASM-LABEL: stack_fallback_indirect:
; ASM: PUSHGO
; ASM: POP 0, 0
define i64 @stack_fallback_indirect(ptr %callee) nounwind {
  %result = tail call i64 %callee(
      i64 0, i64 1, i64 2, i64 3, i64 4, i64 5, i64 6, i64 7, i64 8,
      i64 9, i64 10, i64 11, i64 12, i64 13, i64 14, i64 15, i64 16)
  ret i64 %result
}

; RELOC-LABEL: Relocations [
; RELOC-NEXT: ]

; OBJ-LABEL: <indirect_tail>:
; OBJ: GO r255
; OBJ-LABEL: <function_pointer_recursion>:
; OBJ: GO r255
; OBJ-LABEL: <callee_argument_overlap>:
; OBJ: GO r255
; OBJ-LABEL: <forwarded_sret_indirect>:
; OBJ: GO r255
; OBJ-LABEL: <stack_value_overlap>:
; OBJ: GO r255
; OBJ-LABEL: <stack_fallback_indirect>:
; OBJ: PUSHGO
