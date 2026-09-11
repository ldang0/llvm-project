; RUN: split-file %s %t
; RUN: not llc -mtriple=mmix -O0 -verify-machineinstrs %t/PUSHJ.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=PUSHJ
; RUN: not llc -mtriple=mmix -O0 -verify-machineinstrs %t/PUSHGO.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=PUSHGO
; RUN: not llc -mtriple=mmix -O0 -verify-machineinstrs %t/POP.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=POP

; PUSHJ: LLVM ERROR: MMIX instruction 'PUSHJ' is only permitted in module-level inline assembly
; PUSHGO: LLVM ERROR: MMIX instruction 'PUSHGO' is only permitted in module-level inline assembly
; POP: LLVM ERROR: MMIX instruction 'POP' is only permitted in module-level inline assembly

;--- PUSHJ.ll
define void @rejected() {
  call void asm sideeffect "PUSHJ r31, target", ""()
  ret void
}

;--- PUSHGO.ll
define void @rejected() {
  call void asm sideeffect "pushgo r31, r0, 0", ""()
  ret void
}

;--- POP.ll
define void @rejected() {
  call void asm sideeffect "continuation:\0A\09POP 0, 0", ""()
  ret void
}
