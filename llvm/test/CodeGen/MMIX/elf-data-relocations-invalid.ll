; RUN: split-file %s %t

; RUN: not --crash llc -mtriple=mmix-unknown-elf -filetype=asm \
; RUN:   %t/symbolic-i24.ll -o %t/symbolic-i24.s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=I24
; RUN: not test -e %t/symbolic-i24.s
; RUN: not --crash llc -mtriple=mmix-unknown-elf -filetype=obj \
; RUN:   %t/symbolic-i24.ll -o %t/symbolic-i24.o 2>&1 \
; RUN:   | FileCheck %s --check-prefix=I24
; RUN: not test -e %t/symbolic-i24.o

; RUN: not --crash llc -mtriple=mmix-unknown-elf -filetype=asm \
; RUN:   %t/symbolic-i7-aggregate.ll -o %t/symbolic-i7-aggregate.s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=I7
; RUN: not test -e %t/symbolic-i7-aggregate.s
; RUN: not --crash llc -mtriple=mmix-unknown-elf -filetype=obj \
; RUN:   %t/symbolic-i7-aggregate.ll -o %t/symbolic-i7-aggregate.o 2>&1 \
; RUN:   | FileCheck %s --check-prefix=I7
; RUN: not test -e %t/symbolic-i7-aggregate.o

; RUN: not --crash llc -mtriple=mmix-unknown-elf -filetype=asm \
; RUN:   %t/symbolic-i48-aggregate.ll -o %t/symbolic-i48-aggregate.s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=I48
; RUN: not test -e %t/symbolic-i48-aggregate.s
; RUN: not --crash llc -mtriple=mmix-unknown-elf -filetype=obj \
; RUN:   %t/symbolic-i48-aggregate.ll -o %t/symbolic-i48-aggregate.o 2>&1 \
; RUN:   | FileCheck %s --check-prefix=I48
; RUN: not test -e %t/symbolic-i48-aggregate.o

; RUN: not --crash llc -mtriple=mmix-unknown-elf -filetype=asm \
; RUN:   --output-asm-variant=1 %t/mmixal-symbolic-i24.ll \
; RUN:   -o %t/mmixal-symbolic-i24.s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=MMIXAL-I24
; RUN: not test -e %t/mmixal-symbolic-i24.s

; RUN: llc -mtriple=mmix-unknown-elf -filetype=asm -asm-verbose=false \
; RUN:   %t/supported.ll -o - | FileCheck %s --check-prefix=SUPPORTED

; I24: LLVM ERROR: MMIX symbolic initializer for global 'symbolic_i24'
; I24-SAME: uses unreviewed i24 storage;
; I24-SAME: supported integer widths are i8, i16, i32, and i64
; I7: LLVM ERROR: MMIX symbolic initializer for global 'symbolic_i7_aggregate'
; I7-SAME: uses unreviewed i7 storage;
; I7-SAME: supported integer widths are i8, i16, i32, and i64
; I48: LLVM ERROR: MMIX symbolic initializer for global 'symbolic_i48_aggregate'
; I48-SAME: uses unreviewed i48 storage;
; I48-SAME: supported integer widths are i8, i16, i32, and i64
; MMIXAL-I24: LLVM ERROR: MMIX symbolic initializer for global 'symbolic_i24'
; MMIXAL-I24-SAME: uses unreviewed i24 storage;
; MMIXAL-I24-SAME: supported integer widths are i8, i16, i32, and i64

; SUPPORTED-LABEL: symbolic_i8:
; SUPPORTED-NEXT:  .byte target
; SUPPORTED-LABEL: symbolic_i16:
; SUPPORTED-NEXT:  .2byte target
; SUPPORTED-LABEL: symbolic_i32:
; SUPPORTED-NEXT:  .4byte target
; SUPPORTED-LABEL: symbolic_i64:
; SUPPORTED-NEXT:  .8byte target
; SUPPORTED-LABEL: constant_i7:
; SUPPORTED-NEXT:  .byte 85
; SUPPORTED-LABEL: constant_i24:
; SUPPORTED-NEXT:  .2byte 258
; SUPPORTED-NEXT:  .byte 3
; SUPPORTED-NEXT:  .space 1
; SUPPORTED-LABEL: constant_i48:
; SUPPORTED-NEXT:  .4byte 287454020
; SUPPORTED-NEXT:  .2byte 21862
; SUPPORTED-NEXT:  .space 2

;--- symbolic-i24.ll
target triple = "mmix-unknown-elf"

@target = external global i64
@symbolic_i24 = global i24 ptrtoint (ptr @target to i24)

;--- symbolic-i7-aggregate.ll
target triple = "mmix-unknown-elf"

@target = external global i64
@symbolic_i7_aggregate = global [1 x i7] [
  i7 ptrtoint (ptr @target to i7)
]

;--- symbolic-i48-aggregate.ll
target triple = "mmix-unknown-elf"

@target = external global i64
@symbolic_i48_aggregate = global { i8, i48 } {
  i8 0,
  i48 ptrtoint (ptr @target to i48)
}

;--- mmixal-symbolic-i24.ll
target triple = "mmix-unknown-elf"

@target = global i64 0
@symbolic_i24 = global i24 ptrtoint (ptr @target to i24)

define void @Main() {
entry:
  br label %loop

loop:
  br label %loop
}

;--- supported.ll
target triple = "mmix-unknown-elf"

@target = global i64 0
@symbolic_i8 = global i8 ptrtoint (ptr @target to i8)
@symbolic_i16 = global i16 ptrtoint (ptr @target to i16)
@symbolic_i32 = global i32 ptrtoint (ptr @target to i32)
@symbolic_i64 = global i64 ptrtoint (ptr @target to i64)
@constant_i7 = global i7 85
@constant_i24 = global i24 66051
@constant_i48 = global i48 18838586676582
