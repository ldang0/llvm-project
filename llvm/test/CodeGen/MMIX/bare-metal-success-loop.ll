; XFAIL: *
; FIXME: Remove XFAIL once MMIX assembly generation is implemented.
;
; RUN: llc -mtriple=mmix-unknown-elf -filetype=asm %s -o - | FileCheck %s
;
; This is an LLVM-only output contract test. It must not invoke MMIXAL or QEMU.

; CHECK: .text
; CHECK-LABEL: Main:
; CHECK: JMP bare_metal_success
; CHECK-LABEL: bare_metal_success:
; CHECK-NEXT: JMP bare_metal_success

target triple = "mmix-unknown-elf"

define void @Main() {
entry:
  br label %bare_metal_success

bare_metal_success:
  br label %bare_metal_success
}
