; RUN: split-file %s %t
; RUN: not llc -mtriple=mmix -O0 %t/CASE0.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=CASE0
; RUN: not llc -mtriple=mmix -O0 %t/CASE1.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=CASE1
; RUN: not llc -mtriple=mmix -O0 %t/CASE2.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=CASE2
; RUN: not llc -mtriple=mmix -O0 %t/CASE3.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=CASE3
; RUN: not llc -mtriple=mmix -O0 %t/CASE4.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=CASE4
; RUN: not llc -mtriple=mmix -O0 %t/PUT-RJ.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=PUT-RJ
; RUN: not llc -mtriple=mmix -O0 %t/PUT-RG.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=PUT-RG
; RUN: not llc -mtriple=mmix -O0 %t/PUT-RL.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=PUT-RL
; RUN: not llc -mtriple=mmix -O0 %t/PUT-RA.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=PUT-RA
; RUN: not llc -mtriple=mmix -O0 %t/PUT-RN.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=PUT-RN
; RUN: not llc -mtriple=mmix -O0 %t/PUT-RO.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=PUT-RO
; RUN: not llc -mtriple=mmix -O0 %t/PUT-RS.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=PUT-RS

; CASE0: error: could not allocate input reg for constraint '{rH}'

;--- CASE0.ll
define i64 @special_register_value(i64 %x) {
  %r = call i64 asm sideeffect "OR $0, $1, 0", "=r,{rH}"(i64 %x)
  ret i64 %r
}

; CASE1: error: could not allocate input reg for constraint '{r255}'

;--- CASE1.ll
define i64 @reserved_register_value(i64 %x) {
  %r = call i64 asm sideeffect "OR $0, $1, 0", "=r,{r255}"(i64 %x)
  ret i64 %r
}

; CASE2: error: could not allocate output register for constraint '{r30}'

;--- CASE2.ll
define i64 @return_state_register(i64 %x) {
  %r = call i64 asm sideeffect "OR $0, $1, 0", "={r30},r"(i64 %x)
  ret i64 %r
}

; CASE3: LLVM ERROR: MMIX inline assembly may not clobber register 'rA' in an ordinary function

;--- CASE3.ll
define void @unsafe_special_clobber() {
  call void asm sideeffect "SWYM 0, 0, 0", "~{rA}"()
  ret void
}

; CASE4: LLVM ERROR: MMIX inline assembly may not clobber register 'r254' in an ordinary function

;--- CASE4.ll
define void @stack_pointer_clobber() {
  call void asm sideeffect "SWYM 0, 0, 0", "~{r254}"()
  ret void
}

; PUT-RJ: LLVM ERROR: MMIX instruction 'PUT rJ' is only permitted in module-level inline assembly

;--- PUT-RJ.ll
define void @test() {
  call void asm sideeffect "PUT rJ, r0", ""()
  ret void
}

; PUT-RG: LLVM ERROR: MMIX instruction 'PUT rG' is only permitted in module-level inline assembly

;--- PUT-RG.ll
define void @test() {
  call void asm sideeffect "put rG, r0", ""()
  ret void
}

; PUT-RL: LLVM ERROR: MMIX instruction 'PUT rL' is only permitted in module-level inline assembly

;--- PUT-RL.ll
define void @test() {
  call void asm sideeffect "label:\0A\09PUT rL, r0", ""()
  ret void
}

; PUT-RA: LLVM ERROR: MMIX instruction 'PUT rA' is only permitted in module-level inline assembly

;--- PUT-RA.ll
define void @test() {
  call void asm sideeffect "PUT rA, r0", ""()
  ret void
}

; PUT-RN: LLVM ERROR: MMIX instruction 'PUT rN' is only permitted in module-level inline assembly

;--- PUT-RN.ll
define void @test() {
  call void asm sideeffect "PUT rN, r0", ""()
  ret void
}

; PUT-RO: LLVM ERROR: MMIX instruction 'PUT rO' is only permitted in module-level inline assembly

;--- PUT-RO.ll
define void @test() {
  call void asm sideeffect "PUT rO, r0", ""()
  ret void
}

; PUT-RS: LLVM ERROR: MMIX instruction 'PUT rS' is only permitted in module-level inline assembly

;--- PUT-RS.ll
define void @test() {
  call void asm sideeffect "PUT rS, r0", ""()
  ret void
}
