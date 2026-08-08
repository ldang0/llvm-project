; RUN: split-file %s %t
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm \
; RUN:   --output-asm-variant=1 %t/missing.ll -o %t/missing.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=MISSING
; RUN: test ! -s %t/missing.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm \
; RUN:   --output-asm-variant=1 %t/declaration.ll -o %t/declaration.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=DECLARATION
; RUN: test ! -s %t/declaration.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm \
; RUN:   --output-asm-variant=1 %t/object.ll -o %t/object.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=NONFUNCTION
; RUN: test ! -s %t/object.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm \
; RUN:   --output-asm-variant=1 %t/alias.ll -o %t/alias.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=NONFUNCTION
; RUN: test ! -s %t/alias.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm \
; RUN:   --output-asm-variant=1 %t/weak.ll -o %t/weak.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=WEAK
; RUN: test ! -s %t/weak.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm \
; RUN:   --output-asm-variant=1 %t/return-type.ll -o %t/return-type.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=TYPE
; RUN: test ! -s %t/return-type.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm \
; RUN:   --output-asm-variant=1 %t/arguments.ll -o %t/arguments.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=TYPE
; RUN: test ! -s %t/arguments.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm \
; RUN:   --output-asm-variant=1 %t/varargs.ll -o %t/varargs.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=VARARGS
; RUN: test ! -s %t/varargs.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm \
; RUN:   --output-asm-variant=1 %t/calling-convention.ll \
; RUN:   -o %t/calling-convention.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=CALLING-CONVENTION
; RUN: test ! -s %t/calling-convention.mms
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm \
; RUN:   --output-asm-variant=1 %t/reachable-return.ll \
; RUN:   -o %t/reachable-return.mms 2>&1 \
; RUN:   | FileCheck %s --check-prefix=REACHABLE-RETURN
; RUN: test ! -s %t/reachable-return.mms
; RUN: llc -mtriple=mmix-unknown-elf -filetype=asm \
; RUN:   --output-asm-variant=1 %t/unreachable-return.ll -o - \
; RUN:   | FileCheck %s --check-prefix=UNREACHABLE-RETURN

; MISSING: MMIXAL bare-metal module has no entry named 'Main'
; DECLARATION: MMIXAL bare-metal entry 'Main' must be a definition
; NONFUNCTION: MMIXAL bare-metal entry 'Main' must be a function definition
; WEAK: MMIXAL bare-metal entry 'Main' must be a strong definition
; TYPE: MMIXAL bare-metal entry 'Main' must have type 'void ()'
; VARARGS: MMIXAL bare-metal entry 'Main' must not be variadic
; CALLING-CONVENTION: MMIXAL bare-metal entry 'Main' must use the C calling convention
; REACHABLE-RETURN: MMIXAL bare-metal entry 'Main' must not have a reachable return
; UNREACHABLE-RETURN: Main IS @

;--- missing.ll
target triple = "mmix-unknown-elf"

define void @ordinary() {
  ret void
}

;--- declaration.ll
target triple = "mmix-unknown-elf"

declare void @Main()

;--- object.ll
target triple = "mmix-unknown-elf"

@Main = global i8 0

;--- alias.ll
target triple = "mmix-unknown-elf"

@Main = alias void (), ptr @entry_target

define void @entry_target() {
  unreachable
}

;--- weak.ll
target triple = "mmix-unknown-elf"

define weak void @Main() {
  unreachable
}

;--- return-type.ll
target triple = "mmix-unknown-elf"

define i64 @Main() {
  unreachable
}

;--- arguments.ll
target triple = "mmix-unknown-elf"

define void @Main(i64 %argument) {
  unreachable
}

;--- varargs.ll
target triple = "mmix-unknown-elf"

define void @Main(...) {
  unreachable
}

;--- calling-convention.ll
target triple = "mmix-unknown-elf"

define fastcc void @Main() {
  unreachable
}

;--- reachable-return.ll
target triple = "mmix-unknown-elf"

define void @Main() {
  ret void
}

;--- unreachable-return.ll
target triple = "mmix-unknown-elf"

define void @Main() {
entry:
  br label %loop

loop:
  br label %loop

dead:
  ret void
}
