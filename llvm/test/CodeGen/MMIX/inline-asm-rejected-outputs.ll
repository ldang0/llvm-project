; RUN: split-file %s %t
; RUN: not llc -mtriple=mmix -O0 -verify-machineinstrs %t/scalar.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=SCALAR --implicit-check-not="Bad machine code" --implicit-check-not="Stack dump"
; RUN: not llc -mtriple=mmix -O2 -verify-machineinstrs %t/scalar.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=SCALAR --implicit-check-not="Bad machine code" --implicit-check-not="Stack dump"
; RUN: not llc -mtriple=mmix -O0 -verify-machineinstrs %t/read-write.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=READ-WRITE --implicit-check-not="Bad machine code" --implicit-check-not="Stack dump"
; RUN: not llc -mtriple=mmix -O2 -verify-machineinstrs %t/read-write.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=READ-WRITE --implicit-check-not="Bad machine code" --implicit-check-not="Stack dump"
; RUN: not llc -mtriple=mmix -O0 -verify-machineinstrs %t/memory.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=MEMORY --implicit-check-not="Bad machine code" --implicit-check-not="Stack dump"
; RUN: not llc -mtriple=mmix -O2 -verify-machineinstrs %t/memory.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=MEMORY --implicit-check-not="Bad machine code" --implicit-check-not="Stack dump"
; RUN: not llc -mtriple=mmix -O0 -verify-machineinstrs %t/address-space.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ADDRESS-SPACE --implicit-check-not="Bad machine code" --implicit-check-not="Stack dump"
; RUN: not llc -mtriple=mmix -O2 -verify-machineinstrs %t/address-space.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ADDRESS-SPACE --implicit-check-not="Bad machine code" --implicit-check-not="Stack dump"
; RUN: not llc -mtriple=mmix -O0 -verify-machineinstrs %t/clobber.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=CLOBBER --implicit-check-not="Bad machine code" --implicit-check-not="Stack dump"
; RUN: not llc -mtriple=mmix -O2 -verify-machineinstrs %t/clobber.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=CLOBBER --implicit-check-not="Bad machine code" --implicit-check-not="Stack dump"

; Rejected asm must stop before consumed outputs become undefined virtual
; registers. Both unoptimized and optimized selection must reject without a crash.

; SCALAR: LLVM ERROR: MMIX instruction 'TRAP' is only permitted in module-level inline assembly

;--- scalar.ll
define i64 @test() {
  %r = call i64 asm sideeffect "TRAP 1,0,0", "=r,~{memory}"()
  ret i64 %r
}

; READ-WRITE: LLVM ERROR: MMIX instruction 'TRAP' is only permitted in module-level inline assembly

;--- read-write.ll
define i64 @test(i64 %a, i64 %b) {
  %r = call { i64, i64 } asm sideeffect "TRAP 1,0,0", "={r231},={r237},0,1,~{memory}"(i64 %a, i64 %b)
  %x = extractvalue { i64, i64 } %r, 0
  %y = extractvalue { i64, i64 } %r, 1
  %sum = add i64 %x, %y
  ret i64 %sum
}

; MEMORY: LLVM ERROR: MMIX has no non-offsettable inline assembly memory operand

;--- memory.ll
define i64 @test(ptr %p) {
  %r = call i64 asm sideeffect "LDOU $0, $1", "=r,*V"(ptr elementtype(i64) %p)
  ret i64 %r
}

; ADDRESS-SPACE: LLVM ERROR: MMIX inline assembly does not support memory or address operands in nonzero address spaces

;--- address-space.ll
define i64 @test(ptr addrspace(1) %p) {
  %r = call i64 asm sideeffect "LDOU $0, $1", "=r,*m"(ptr addrspace(1) elementtype(i64) %p)
  ret i64 %r
}

; CLOBBER: LLVM ERROR: MMIX inline assembly may not clobber register 'r254' in an ordinary function

;--- clobber.ll
define i64 @test(i64 %v) {
  %r = call i64 asm sideeffect "OR $0, $1, 0", "=r,r,~{r254}"(i64 %v)
  ret i64 %r
}
