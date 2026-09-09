; RUN: llc -mtriple=mmix-unknown-elf -filetype=asm -asm-verbose=false %s -o - \
; RUN:   | FileCheck %s
;
; This LLVM-only test checks canonical assembly. MMIXAL compatibility and
; external execution belong to the separate bare-metal validation contract.

; CHECK: .text
; CHECK: .globl Main
; CHECK-LABEL: Main:
; CHECK-NEXT: [[LOOP:\.[A-Za-z0-9_.$]+]]:
; CHECK-NEXT: JMP [[LOOP]]

target triple = "mmix-unknown-elf"

define void @Main() nounwind {
entry:
  br label %bare_metal_success

bare_metal_success:
  br label %bare_metal_success
}
