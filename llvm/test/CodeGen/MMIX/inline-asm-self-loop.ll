; RUN: llc -mtriple=mmix -O0 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=mmix -O2 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=mmix -O0 -verify-machineinstrs -filetype=obj %s -o %t.o
; RUN: llvm-objdump -d --no-print-imm-hex %t.o | FileCheck %s --check-prefix=OBJ
; RUN: llc -mtriple=mmix -O2 -verify-machineinstrs -filetype=obj %s -o %t.opt.o
; RUN: llvm-objdump -d --no-print-imm-hex %t.opt.o | FileCheck %s --check-prefix=OBJ

; Zero-byte inline asm must not make a zero-displacement self-loop backward.
; CHECK-LABEL: empty:
; CHECK: [[EMPTY:.LBB[0-9]+_[0-9]+]]:
; CHECK: #APP
; CHECK: #NO_APP
; CHECK-NEXT: JMP [[EMPTY]]
; OBJ-LABEL: <empty>:
; OBJ: f0 00 00 00{{.*}}JMP 0
define void @empty() nounwind {
entry:
  br label %loop
loop:
  call void asm sideeffect "", "~{memory}"()
  br label %loop
}

; CHECK-LABEL: comments:
; CHECK: [[COMMENTS:.LBB[0-9]+_[0-9]+]]:
; CHECK: #APP
; CHECK: #NO_APP
; CHECK-NEXT: JMP [[COMMENTS]]
; OBJ-LABEL: <comments>:
; OBJ: f0 00 00 00{{.*}}JMP 0
define void @comments() nounwind {
entry:
  br label %loop
loop:
  call void asm sideeffect "  # no instruction\0A\09", "~{memory}"()
  br label %loop
}

; A real instruction before the branch still requires a negative displacement.
; CHECK-LABEL: instruction:
; CHECK: [[INSTRUCTION:.LBB[0-9]+_[0-9]+]]:
; CHECK: SWYM
; CHECK: #NO_APP
; CHECK-NEXT: JMPB [[INSTRUCTION]]
; OBJ-LABEL: <instruction>:
; OBJ: fd 00 00 00{{.*}}SWYM
; OBJ-NEXT: {{.*}}f1 ff ff ff{{.*}}JMPB -1
define void @instruction() nounwind {
entry:
  br label %loop
loop:
  call void asm sideeffect "SWYM 0,0,0", "~{memory}"()
  br label %loop
}
