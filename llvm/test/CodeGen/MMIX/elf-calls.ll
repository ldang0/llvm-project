; RUN: llc -mtriple=mmix-unknown-elf -verify-machineinstrs \
; RUN:   -stop-after=mmix-isel %s -o - | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=mmix-unknown-elf -verify-machineinstrs \
; RUN:   -stop-after=postrapseudos %s -o - | FileCheck %s --check-prefix=POSTRA
; RUN: llc -mtriple=mmix-unknown-elf -verify-machineinstrs -filetype=asm \
; RUN:   %s -o - | FileCheck %s --check-prefix=ASM
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=obj %s -o %t.o 2>&1 \
; RUN:   | FileCheck %s --check-prefix=OBJECT
; RUN: test ! -s %t.o

target triple = "mmix-unknown-elf"

declare void @declared()
declare extern_weak void @weak_declared()

define internal void @backward_target() {
  ret void
}

; Stable same-section callees retain the layout-selected direct-call path.
; ISEL-LABEL: name: call_backward
; ISEL-NOT:   LOAD_CALL_ADDR
; ISEL:       CALL_STATE @backward_target, csr_mmix
; POSTRA-LABEL: name: call_backward
; POSTRA-NOT:   LOAD_CALL_ADDR
; POSTRA:       PseudoPUSHJ $r31, @backward_target, csr_mmix
; ASM-LABEL: call_backward:
; ASM:       PUSHJB r31, backward_target
define void @call_backward() {
  call void @backward_target()
  ret void
}

; ISEL-LABEL: name: call_forward
; ISEL-NOT:   LOAD_CALL_ADDR
; ISEL:       CALL_STATE @forward_target, csr_mmix
; POSTRA-LABEL: name: call_forward
; POSTRA-NOT:   LOAD_CALL_ADDR
; POSTRA:       PseudoPUSHJ $r31, @forward_target, csr_mmix
; ASM-LABEL: call_forward:
; ASM:       PUSHJ r31, forward_target
define void @call_forward() {
  call void @forward_target()
  ret void
}

define internal void @forward_target() {
  ret void
}

define internal void @section_target() section ".text.separate" {
  ret void
}

; Inter-section, declared, externally visible, and weak callees retain a
; symbolic direct-call operand alongside the scratch used by text output.
; ISEL-LABEL: name: call_other_section
; ISEL:       [[SECTION_ADDR:%[0-9]+]]:{{[^ ]+}} = LOAD_CALL_ADDR @section_target
; ISEL-NEXT:  DIRECT_CALL_STATE @section_target, killed [[SECTION_ADDR]], csr_mmix
; POSTRA-LABEL: name: call_other_section
; POSTRA:       [[SECTION_REG:\$r[0-9]+]] = LOAD_CALL_ADDR @section_target
; POSTRA-NEXT:  PseudoDirectCall $r31, @section_target, killed [[SECTION_REG]], csr_mmix
; ASM-LABEL: call_other_section:
; ASM:       SETH [[SECTION_TEXT:r[0-9]+]], (section_target>>48)&65535
; ASM:       PUSHGO r31, [[SECTION_TEXT]], 0
define void @call_other_section() {
  call void @section_target()
  ret void
}

; ISEL-LABEL: name: call_declaration
; ISEL:       [[DECL_ADDR:%[0-9]+]]:{{[^ ]+}} = LOAD_CALL_ADDR @declared
; ISEL-NEXT:  DIRECT_CALL_STATE @declared, killed [[DECL_ADDR]], csr_mmix
; POSTRA-LABEL: name: call_declaration
; POSTRA:       PseudoDirectCall $r31, @declared, killed $r{{[0-9]+}}, csr_mmix
; ASM-LABEL: call_declaration:
; ASM:       SETH [[DECL_TEXT:r[0-9]+]], (declared>>48)&65535
; ASM:       PUSHGO r31, [[DECL_TEXT]], 0
define void @call_declaration() {
  call void @declared()
  ret void
}

define dso_preemptable void @visible_target() {
  ret void
}

; ISEL-LABEL: name: call_visible
; ISEL:       DIRECT_CALL_STATE @visible_target, {{.*}}csr_mmix
; POSTRA-LABEL: name: call_visible
; POSTRA:       PseudoDirectCall $r31, @visible_target, {{.*}}csr_mmix
define void @call_visible() {
  call void @visible_target()
  ret void
}

; ISEL-LABEL: name: call_weak
; ISEL:       DIRECT_CALL_STATE @weak_declared, {{.*}}csr_mmix
; POSTRA-LABEL: name: call_weak
; POSTRA:       PseudoDirectCall $r31, @weak_declared, {{.*}}csr_mmix
define void @call_weak() {
  call void @weak_declared()
  ret void
}

; A signed constant attached to a function symbol survives both producer
; boundaries. The eventual ELF helper therefore receives one S+A expression.
; ISEL-LABEL: name: call_with_addend
; ISEL:       [[ADDEND_ADDR:%[0-9]+]]:{{[^ ]+}} = LOAD_CALL_ADDR @declared - 12
; ISEL-NEXT:  DIRECT_CALL_STATE @declared - 12, killed [[ADDEND_ADDR]], csr_mmix
; POSTRA-LABEL: name: call_with_addend
; POSTRA:       PseudoDirectCall $r31, @declared - 12, killed $r{{[0-9]+}}, csr_mmix
; ASM-LABEL: call_with_addend:
; ASM:       SETH [[ADDEND_TEXT:r[0-9]+]], ((declared-12)>>48)&65535
; ASM:       PUSHGO r31, [[ADDEND_TEXT]], 0
define void @call_with_addend() {
  call void getelementptr (i8, ptr @declared, i64 -12)()
  ret void
}

; Register-indirect calls never enter the symbolic direct-call path.
; ISEL-LABEL: name: call_indirect
; ISEL-NOT:   LOAD_CALL_ADDR
; ISEL-NOT:   DIRECT_CALL_STATE
; ISEL:       CALL_STATE %{{[0-9]+}}, csr_mmix
; POSTRA-LABEL: name: call_indirect
; POSTRA-NOT:   LOAD_CALL_ADDR
; POSTRA-NOT:   PseudoDirectCall
; POSTRA:       PseudoPUSHGO $r31, killed $r231, 0, csr_mmix
; ASM-LABEL: call_indirect:
; ASM:       PUSHGO r31, r231, 0
define void @call_indirect(ptr %callee) {
  call void %callee()
  ret void
}

; Object emission remains deliberately gated until the complete CodeGen ELF
; contract is assembled in the next milestone.
; OBJECT: llc: error: target does not support generation of this file type
