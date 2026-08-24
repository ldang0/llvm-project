; RUN: split-file %s %t

; Non-static models are the boundary that would otherwise introduce dynamic,
; GOT, or PLT address forms.
; RUN: not --crash llc -mtriple=mmix-unknown-elf -filetype=obj \
; RUN:   -relocation-model=pic %t/options.ll -o %t/pic.o 2>&1 \
; RUN:   | FileCheck %s --check-prefixes=RELOCATION,COMMON
; RUN: test ! -s %t/pic.o
; RUN: not --crash llc -mtriple=mmix-unknown-elf -filetype=obj \
; RUN:   -relocation-model=dynamic-no-pic %t/options.ll -o %t/dynamic.o 2>&1 \
; RUN:   | FileCheck %s --check-prefixes=RELOCATION,COMMON
; RUN: test ! -s %t/dynamic.o
; RUN: not --crash llc -mtriple=mmix-unknown-elf -filetype=obj \
; RUN:   -code-model=large %t/options.ll -o %t/large.o 2>&1 \
; RUN:   | FileCheck %s --check-prefixes=CODE-MODEL,COMMON
; RUN: test ! -s %t/large.o

; RUN: not llc -mtriple=mmix-unknown-elf -filetype=obj \
; RUN:   %t/tls.ll -o %t/tls.o 2>&1 \
; RUN:   | FileCheck %s --check-prefixes=TLS,COMMON
; RUN: test ! -s %t/tls.o
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=obj \
; RUN:   %t/address-space.ll -o %t/address-space.o 2>&1 \
; RUN:   | FileCheck %s --check-prefixes=ADDRESS-SPACE,COMMON
; RUN: test ! -s %t/address-space.o

; RUN: not llc -mtriple=mmix-unknown-elf -filetype=obj \
; RUN:   %t/calling-convention.ll -o %t/calling-convention.o 2>&1 \
; RUN:   | FileCheck %s --check-prefixes=CALLING-CONVENTION,COMMON
; RUN: test ! -s %t/calling-convention.o
; RUN: not --crash llc -mtriple=mmix-unknown-elf -filetype=obj \
; RUN:   %t/symbolic-i24.ll -o %t/symbolic-i24.o 2>&1 \
; RUN:   | FileCheck %s --check-prefixes=SYMBOLIC-I24,COMMON
; RUN: test ! -s %t/symbolic-i24.o

; Canonical module assembly remains available for Task 50's reviewed fields,
; but unsupported expressions and relocation identities remain atomic errors.
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=obj \
; RUN:   %t/split-address.ll -o %t/split-address.o 2>&1 \
; RUN:   | FileCheck %s --check-prefixes=SPLIT-ADDRESS,COMMON
; RUN: test ! -s %t/split-address.o
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=obj \
; RUN:   %t/symbol-difference.ll -o %t/symbol-difference.o 2>&1 \
; RUN:   | FileCheck %s --check-prefixes=SYMBOL-DIFFERENCE,COMMON
; RUN: test ! -s %t/symbol-difference.o
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=obj \
; RUN:   %t/intermediate-relocation.ll -o %t/intermediate-relocation.o 2>&1 \
; RUN:   | FileCheck %s --check-prefixes=INTERMEDIATE,COMMON
; RUN: test ! -s %t/intermediate-relocation.o
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=obj \
; RUN:   %t/gnu-register-relocations.ll -o %t/gnu-register-relocations.o 2>&1 \
; RUN:   | FileCheck %s --check-prefixes=GNU-REGISTER,COMMON
; RUN: test ! -s %t/gnu-register-relocations.o

; RUN: not llc -mtriple=mmix-unknown-elf -filetype=obj \
; RUN:   %t/misaligned-branch.ll -o %t/misaligned-branch.o 2>&1 \
; RUN:   | FileCheck %s --check-prefixes=BRANCH,COMMON
; RUN: test ! -s %t/misaligned-branch.o
; RUN: not llc -mtriple=mmix-unknown-elf -filetype=obj \
; RUN:   %t/misaligned-call.ll -o %t/misaligned-call.o 2>&1 \
; RUN:   | FileCheck %s --check-prefixes=CALL,COMMON
; RUN: test ! -s %t/misaligned-call.o

; COMMON-NOT: target does not support generation of this file type
; RELOCATION: LLVM ERROR: MMIX supports only the static relocation model
; CODE-MODEL: LLVM ERROR: MMIX supports only the small code model
; TLS: LLVM ERROR: MMIX does not support thread-local storage in function 'owner'
; ADDRESS-SPACE: LLVM ERROR: MMIX does not support nonzero address spaces in function 'owner'
; CALLING-CONVENTION: LLVM ERROR: MMIX supports only C and Fast calling conventions in function 'owner'
; SYMBOLIC-I24: LLVM ERROR: MMIX symbolic initializer for global 'symbolic_i24'
; SYMBOLIC-I24-SAME: uses unreviewed i24 storage;
; SPLIT-ADDRESS: error: unresolved MMIX split-address expression is not supported; use GETA with '%geta(...)'
; SYMBOL-DIFFERENCE: error: expanding GETA requires one symbol plus an optional addend
; INTERMEDIATE: error: GNU MMIX intermediate relaxation relocations cannot be emitted directly
; GNU-REGISTER-COUNT-4: error: unknown relocation name
; BRANCH: error: MMIX 19-bit terminal target is not instruction aligned
; CALL: error: MMIX direct call target is not instruction aligned

;--- options.ll
target triple = "mmix-unknown-elf"

define void @owner() {
  ret void
}

;--- tls.ll
target triple = "mmix-unknown-elf"

@tls = thread_local global i64 0

define i64 @owner() {
  %value = load i64, ptr @tls
  ret i64 %value
}

;--- address-space.ll
target triple = "mmix-unknown-elf"

@value = addrspace(1) global i64 0

define i64 @owner() {
  %value = load i64, ptr addrspace(1) @value
  ret i64 %value
}

;--- calling-convention.ll
target triple = "mmix-unknown-elf"

define coldcc void @owner() {
  ret void
}

;--- symbolic-i24.ll
target triple = "mmix-unknown-elf"

@external = external global i64
@symbolic_i24 = global i24 ptrtoint (ptr @external to i24)

;--- split-address.ll
target triple = "mmix-unknown-elf"

module asm "SETH r1, (external>>48)&65535"

;--- symbol-difference.ll
target triple = "mmix-unknown-elf"

module asm "GETA r1, %geta(external - local)"
module asm "local:"
module asm "SWYM 0, 0, 0"

;--- intermediate-relocation.ll
target triple = "mmix-unknown-elf"

module asm ".reloc ., R_MMIX_GETA_1, external"

; GNU linker-allocated GREG and LOCAL records conflict with the provisional
; fixed-register ABI and have no canonical LLVM producer.
;--- gnu-register-relocations.ll
target triple = "mmix-unknown-elf"

module asm ".reloc ., R_MMIX_REG_OR_BYTE, external"
module asm ".reloc ., R_MMIX_REG, external"
module asm ".reloc ., R_MMIX_BASE_PLUS_OFFSET, external"
module asm ".reloc ., R_MMIX_LOCAL, external"

;--- misaligned-branch.ll
target triple = "mmix-unknown-elf"

module asm "branch:"
module asm "BN r1, target"
module asm ".set target, branch + 2"

;--- misaligned-call.ll
target triple = "mmix-unknown-elf"

define internal void @callee() {
  ret void
}

define void @owner() {
  call void getelementptr (i8, ptr @callee, i64 2)()
  ret void
}
