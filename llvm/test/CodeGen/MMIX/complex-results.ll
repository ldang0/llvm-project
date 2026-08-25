; RUN: llc -mtriple=mmix -O0 -verify-machineinstrs -stop-after=mmix-isel %s -o - | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=mmix -O2 -verify-machineinstrs -filetype=asm %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=mmix-unknown-elf -O2 -verify-machineinstrs -filetype=obj %s -o /dev/null

target triple = "mmix"

%complex = type { double, double }

declare %complex @make_complex(double, double)
declare void @clobber()

; ISEL-LABEL: name: return_complex
; ISEL:       $r231 = COPY
; ISEL:       $r232 = COPY
; ISEL:       RET_PAIR {{.*}}implicit $r231, implicit $r232
define %complex @return_complex(double %real, double %imaginary) {
  %with_real = insertvalue %complex poison, double %real, 0
  %value = insertvalue %complex %with_real, double %imaginary, 1
  ret %complex %value
}

; ISEL-LABEL: name: add_complex_components
; ISEL:       DIRECT_CALL_STATE @make_complex, {{.*}}implicit-def $r231, implicit-def $r232
; ISEL:       [[REAL:%[0-9]+]]:{{[^ ]+}} = COPY $r231
; ISEL:       [[IMAGINARY:%[0-9]+]]:{{[^ ]+}} = COPY $r232
; ISEL:       FADD {{.*}}[[REAL]], {{.*}}[[IMAGINARY]]
define double @add_complex_components(double %real, double %imaginary) {
  %value = call %complex @make_complex(double %real, double %imaginary)
  %result_real = extractvalue %complex %value, 0
  %result_imaginary = extractvalue %complex %value, 1
  %sum = fadd double %result_real, %result_imaginary
  ret double %sum
}

; ISEL-LABEL: name: call_complex_indirect
; ISEL:       CALL_STATE {{.*}}implicit-def $r231, implicit-def $r232
; ISEL:       COPY $r231
; ISEL:       COPY $r232
define double @call_complex_indirect(ptr %callee, double %real,
                                     double %imaginary) {
  %value = call %complex %callee(double %real, double %imaginary)
  %result = extractvalue %complex %value, 1
  ret double %result
}

; Both result registers retain their ABI locations across the terminal
; transfer, so the caller returns the Complex value without copying it.
; ASM-LABEL: forward_complex:
; ASM-NOT:   PUSH
; ASM:       GO r255
define %complex @forward_complex(double %real, double %imaginary) {
  %value = tail call %complex @make_complex(double %real, double %imaginary)
  ret %complex %value
}

; Explicit volatile storage forces both components through memory across a
; call that clobbers the global result bank.
; ASM-LABEL: spill_complex_across_call:
; ASM:       PUSHGO
; ASM:       STOU
; ASM:       STOU
; ASM:       PUSHGO
; ASM:       LDO
; ASM:       LDO
; ASM:       POP 0, 0
define %complex @spill_complex_across_call(double %real, double %imaginary) {
  %slot = alloca %complex, align 8
  %value = call %complex @make_complex(double %real, double %imaginary)
  store volatile %complex %value, ptr %slot, align 8
  call void @clobber()
  %reloaded = load volatile %complex, ptr %slot, align 8
  ret %complex %reloaded
}
