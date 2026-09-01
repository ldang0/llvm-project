; REQUIRES: mmix

; Verify MMIX bitcode identity without claiming successful LTO code generation.
; RUN: rm -rf %t && split-file %s %t
; RUN: llvm-as %t/canonical.ll -o %t/canonical.bc
; RUN: llvm-as %t/explicit-elf.ll -o %t/explicit-elf.bc
; RUN: ld.lld -m elf64mmix --lto-emit-llvm -e _start %t/canonical.bc \
; RUN:   -o %t/canonical-linked.bc
; RUN: ld.lld -m elf64mmix --lto-emit-llvm -e _start %t/explicit-elf.bc \
; RUN:   -o %t/explicit-elf-linked.bc
; RUN: llvm-dis %t/canonical-linked.bc -o - | FileCheck %s --check-prefix=VALID
; RUN: llvm-dis %t/explicit-elf-linked.bc -o - | FileCheck %s --check-prefix=VALID

; Selecting the MMIX emulation must not reinterpret other, unknown, or malformed
; bitcode as MMIX input.
; RUN: llvm-as %t/little-endian.ll -o %t/little-endian.bc
; RUN: not ld.lld -m elf64mmix --lto-emit-llvm %t/little-endian.bc -o %t/bad 2>&1 \
; RUN:   | FileCheck %s --check-prefix=INCOMPATIBLE
; RUN: llvm-as %t/unknown.ll -o %t/unknown.bc
; RUN: not ld.lld -m elf64mmix --lto-emit-llvm %t/unknown.bc -o %t/bad 2>&1 \
; RUN:   | FileCheck %s --check-prefix=UNKNOWN
; RUN: cp %t/canonical.bc %t/malformed.bc
; RUN: %python -c "with open(r'%t/malformed.bc', 'a') as f: f.truncate(10)"
; RUN: not ld.lld -m elf64mmix %t/malformed.bc -o %t/bad 2>&1 \
; RUN:   | FileCheck %s --check-prefix=MALFORMED

; VALID: target triple = "mmix-unknown-{{(unknown|elf)}}"
; INCOMPATIBLE: error: {{.*}}little-endian.bc is incompatible with elf64mmix
; UNKNOWN: error: {{.*}}could not infer e_machine from bitcode target triple unknown-unknown-unknown
; MALFORMED: error: {{.*}}malformed.bc: Invalid bitcode signature

;--- canonical.ll
target datalayout = "E-m:e-p:64:64-i64:64-n64-S64"
target triple = "mmix-unknown-unknown"

define void @_start() {
  ret void
}

;--- explicit-elf.ll
target datalayout = "E-m:e-p:64:64-i64:64-n64-S64"
target triple = "mmix-unknown-elf"

define void @_start() {
  ret void
}

;--- little-endian.ll
target datalayout = "e-m:e-p:64:64-i64:64-n8:16:32:64-S128"
target triple = "x86_64-unknown-unknown"

define void @_start() {
  ret void
}

;--- unknown.ll
target datalayout = "E-m:e-p:64:64-i64:64-n64-S64"
target triple = "unknown-unknown-unknown"

define void @_start() {
  ret void
}
