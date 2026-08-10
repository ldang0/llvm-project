; RUN: llc -mtriple=mmix -verify-machineinstrs -filetype=asm %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=mmix -verify-machineinstrs -filetype=asm %s -o - | llvm-mc -triple=mmix -filetype=asm -o /dev/null
; RUN: llc -mtriple=mmix -verify-machineinstrs -stop-after=mmix-isel %s -o - | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=mmix -verify-machineinstrs -stop-after=postrapseudos %s -o - | FileCheck %s --check-prefix=MIR
; RUN: not --crash llc -mtriple=mmix -relocation-model=pic -filetype=asm %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=PIC

target triple = "mmix"

@data = external global i64

declare void @callee()

; ASM-LABEL: global_address:
; ASM:      SETH r231, (data>>48)&65535
; ASM-NEXT: INCMH r231, (data>>32)&65535
; ASM-NEXT: INCML r231, (data>>16)&65535
; ASM-NEXT: INCL r231, data&65535
; ISEL-LABEL: name: global_address
; ISEL:       %{{[0-9]+}}:gpr64codegen = LOAD_ADDR @data
; MIR-LABEL: name: global_address
; MIR:       $r231 = LOAD_ADDR @data
define ptr @global_address() {
  ret ptr @data
}

; ASM-LABEL: function_address:
; ASM:      SETH r231, (callee>>48)&65535
; ASM-NEXT: INCMH r231, (callee>>32)&65535
; ASM-NEXT: INCML r231, (callee>>16)&65535
; ASM-NEXT: INCL r231, callee&65535
; ISEL-LABEL: name: function_address
; ISEL:       %{{[0-9]+}}:gpr64codegen = LOAD_ADDR @callee
define ptr @function_address() {
  ret ptr @callee
}

; Materialize a non-folded addend independently without truncating it.
; ASM-LABEL: global_address_addend:
; ASM:      SETL [[OFFSET:r[0-9]+]], 4660
; ASM-NEXT: SETH [[BASE:r[0-9]+]], (data>>48)&65535
; ASM-NEXT: INCMH [[BASE]], (data>>32)&65535
; ASM-NEXT: INCML [[BASE]], (data>>16)&65535
; ASM-NEXT: INCL [[BASE]], data&65535
; ASM-NEXT: ADDU r231, [[BASE]], [[OFFSET]]
define ptr @global_address_addend() {
  ret ptr getelementptr (i8, ptr @data, i64 4660)
}

; ASM-LABEL: block_address:
; ASM:      SETH r231, ([[BLOCK:\.Ltmp[0-9]+]]>>48)&65535
; ASM-NEXT: INCMH r231, ([[BLOCK]]>>32)&65535
; ASM-NEXT: INCML r231, ([[BLOCK]]>>16)&65535
; ASM-NEXT: INCL r231, [[BLOCK]]&65535
; ISEL-LABEL: name: block_address
; ISEL:       %{{[0-9]+}}:gpr64codegen = LOAD_ADDR blockaddress(@block_address, %ir-block.target)
define ptr @block_address() {
entry:
  br label %target

target:
  ret ptr blockaddress(@block_address, %target)
}

; PIC: MMIX supports only the static relocation model
