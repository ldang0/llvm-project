; REQUIRES: mmix

; Link a deliberately restricted llc-produced object through the supported
; static MMIX path. It uses no calls, runtime helpers, GETA, or stubs.
; RUN: rm -rf %t && split-file %s %t
; RUN: llc -mtriple=mmix-unknown-elf -filetype=obj %t/supported.ll \
; RUN:   -o %t/supported.o
; RUN: ld.lld -T %t/layout.lds %t/supported.o -o %t/executable
; RUN: llvm-readobj --file-headers --sections --symbols %t/executable \
; RUN:   | FileCheck %s --check-prefix=STRUCTURE
; RUN: llvm-readobj --relocations %t/executable \
; RUN:   | FileCheck %s --check-prefix=RELOCS --implicit-check-not=R_MMIX_
; RUN: llvm-objdump --no-print-imm-hex -d %t/executable \
; RUN:   | FileCheck %s --check-prefix=DIS
; RUN: llvm-objdump -s --section=.data %t/executable \
; RUN:   | FileCheck %s --check-prefix=DATA

; External addresses and calls are intentionally adjacent negative coverage:
; llc emits the GNU-compatible records, while their linker transformations
; remain work for the next milestone.
; RUN: llc -mtriple=mmix-unknown-elf -filetype=obj %t/unsupported.ll \
; RUN:   -o %t/unsupported.o
; RUN: llvm-mc -triple=mmix -filetype=obj %t/definitions.s \
; RUN:   -o %t/definitions.o
; RUN: not ld.lld --error-limit=0 -e use_external %t/unsupported.o \
; RUN:   %t/definitions.o -o /dev/null 2>&1 \
; RUN:   | FileCheck %s --check-prefix=MILESTONE3

; STRUCTURE:      Format: elf64-mmix
; STRUCTURE:      Type: Executable
; STRUCTURE:      Entry: 0x10000
; STRUCTURE:      Name: .text
; STRUCTURE:      Address: 0x10000
; STRUCTURE:      Name: .data
; STRUCTURE:      Address: 0x20000
; STRUCTURE:      Name: _start
; STRUCTURE:      Value: 0x10000
; STRUCTURE:      Type: Function
; STRUCTURE:      Name: value
; STRUCTURE:      Value: 0x20000
; STRUCTURE:      Type: Object
; STRUCTURE:      Name: value_pointer
; STRUCTURE:      Value: 0x20008
; STRUCTURE:      Type: Object
; RELOCS:      Relocations [
; RELOCS-NEXT: ]

; DIS-LABEL: <_start>:
; DIS:       JMP 0

; DATA:      Contents of section .data:
; DATA-NEXT: 20000 00000000 0000002a 00000000 00020000

; MILESTONE3-DAG: unsupported relocation R_MMIX_PUSHJ_STUBBABLE against symbol external_function: requires MMIX range-extension stub support

;--- layout.lds
ENTRY(_start)
SECTIONS {
  .text 0x10000 : { *(.text) }
  .data 0x20000 : { *(.data) }
  .bss 0x30000 : { *(.bss) }
}

;--- supported.ll
target triple = "mmix-unknown-elf"

@value = global i64 42, align 8
@value_pointer = global ptr @value, align 8

define void @_start() noreturn nounwind {
entry:
  br label %loop

loop:
  br label %loop
}

;--- unsupported.ll
target triple = "mmix-unknown-elf"

@external_data = external global i64
declare void @external_function()

define i64 @use_external() {
entry:
  %value = load volatile i64, ptr @external_data, align 8
  call void @external_function()
  ret i64 %value
}

;--- definitions.s
.data
.global external_data
external_data:
.quad 0
.text
.global external_function
external_function:
POP 0, 0
