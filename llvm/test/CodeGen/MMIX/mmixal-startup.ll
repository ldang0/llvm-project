; RUN: llc -mtriple=mmix-unknown-elf -filetype=asm \
; RUN:   --output-asm-variant=1 %s -o - | FileCheck %s \
; RUN:   --implicit-check-not=PUSHJ --implicit-check-not=PUSHGO \
; RUN:   --implicit-check-not=POP

target triple = "mmix-unknown-elf"

define void @Main() {
entry:
  br label %loop

loop:
  br label %loop
}

; CHECK:      __LLVM_G_SP	GREG #2000000004000000
; CHECK-NEXT: __LLVM_G_FP	GREG 0
; CHECK-NEXT: __LLVM_G_R252	GREG 0
; CHECK:      __LLVM_G_R231	GREG 0
; CHECK-NEXT: LOC #0000000000000100
; CHECK-NEXT: Main	IS @
; CHECK-NEXT: PUT rA, 0
; CHECK-NEXT: PUT rL, 0
; CHECK-NEXT: LOC #0000000000000108
; CHECK-NEXT: [[LOOP:__LLVM_L_F_4D61696E_BB_[0-9]+]]	IS @
; CHECK-NEXT: loop	IS @
; CHECK-NEXT: JMP [[LOOP]]
