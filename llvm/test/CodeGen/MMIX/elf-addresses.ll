; RUN: llc -mtriple=mmix-unknown-elf -verify-machineinstrs \
; RUN:   -stop-after=postrapseudos %s -o - | FileCheck %s --check-prefix=MIR
; RUN: llc -mtriple=mmix-unknown-elf -verify-machineinstrs -filetype=asm \
; RUN:   %s -o - | FileCheck %s --check-prefix=ASM
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=obj %s -o %t.o 2>&1 \
; RUN:   | FileCheck %s --check-prefix=OBJECT
; RUN: test ! -s %t.o

target triple = "mmix-unknown-elf"

@defined_data = internal global [8 x i8] zeroinitializer, align 8
@external_data = external global i8

declare void @external_function()

define internal void @defined_function() {
  ret void
}

; MIR-LABEL: name: defined_data_address
; MIR:       $r231 = LOAD_ADDR @defined_data
; ASM-LABEL: defined_data_address:
; ASM:       SETH r231, (defined_data>>48)&65535
; ASM-NEXT:  INCMH r231, (defined_data>>32)&65535
; ASM-NEXT:  INCML r231, (defined_data>>16)&65535
; ASM-NEXT:  INCL r231, defined_data&65535
define ptr @defined_data_address() {
  ret ptr @defined_data
}

; MIR-LABEL: name: external_data_address
; MIR:       $r231 = LOAD_ADDR @external_data
define ptr @external_data_address() {
  ret ptr @external_data
}

; MIR-LABEL: name: defined_function_address
; MIR:       $r231 = LOAD_ADDR @defined_function
define ptr @defined_function_address() {
  ret ptr @defined_function
}

; MIR-LABEL: name: external_function_address
; MIR:       $r231 = LOAD_ADDR @external_function
define ptr @external_function_address() {
  ret ptr @external_function
}

; A folded positive byte offset remains attached to the static-address pseudo.
; The text path applies every split operation to the complete S+A expression;
; the object path therefore receives one symbol-plus-addend expression too.
; MIR-LABEL: name: positive_address_offset
; MIR:       $r231 = LOAD_ADDR @defined_data + 4660
; ASM-LABEL: positive_address_offset:
; ASM:       SETH r231, ((defined_data+4660)>>48)&65535
; ASM-NEXT:  INCMH r231, ((defined_data+4660)>>32)&65535
; ASM-NEXT:  INCML r231, ((defined_data+4660)>>16)&65535
; ASM-NEXT:  INCL r231, (defined_data+4660)&65535
define ptr @positive_address_offset() {
  ret ptr getelementptr (i8, ptr @defined_data, i64 4660)
}

; The current lowering emits this negative offset as explicit integer
; arithmetic, preserving the complete signed value outside the relocation.
; MIR-LABEL: name: negative_address_offset
; MIR:       [[NEGATIVE_OFFSET:\$r[0-9]+]] = SETL 4660
; MIR-NEXT:  [[NEGATIVE_OFFSET]] = NEGU 0, [[NEGATIVE_OFFSET]]
; MIR-NEXT:  [[NEGATIVE_BASE:\$r[0-9]+]] = LOAD_ADDR @external_data
; MIR-NEXT:  $r231 = ADDU killed [[NEGATIVE_BASE]], killed [[NEGATIVE_OFFSET]]
define ptr @negative_address_offset() {
  ret ptr getelementptr (i8, ptr @external_data, i64 -4660)
}

; MIR-LABEL: name: block_address
; MIR:       $r231 = LOAD_ADDR blockaddress(@block_address, %ir-block.target)
define ptr @block_address() {
entry:
  br label %target

target:
  ret ptr blockaddress(@block_address, %target)
}

; Unresolved direct calls use LOAD_CALL_ADDR before post-RA and therefore
; remain outside the static-address GETA path.
; MIR-LABEL: name: direct_call
; MIR-NOT:   LOAD_ADDR
; MIR:       $r{{[0-9]+}} = SETH target-flags(mmix-abs-hi) @external_function
; MIR:       PseudoPUSHGO
define void @direct_call() {
  call void @external_function()
  ret void
}

; Indirect calls already hold their target in a register.
; MIR-LABEL: name: indirect_call
; MIR-NOT:   LOAD_ADDR
; MIR:       PseudoPUSHGO
define void @indirect_call(ptr %callee) {
  call void %callee()
  ret void
}

; OBJECT: llc: error: target does not support generation of this file type
