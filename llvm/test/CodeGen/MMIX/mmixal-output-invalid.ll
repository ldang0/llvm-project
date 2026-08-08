; RUN: not llc -mtriple=mmix-unknown-elf -filetype=asm \
; RUN:   --output-asm-variant=42 %s -o %t.unknown 2>&1 \
; RUN:   | FileCheck %s --check-prefix=UNKNOWN
; RUN: test ! -s %t.unknown

; UNKNOWN: error: unable to create instruction printer for target triple 'mmix-unknown-unknown-elf' with assembly variant 42
; UNKNOWN-NOT: PLEASE submit a bug report

target triple = "mmix-unknown-elf"

define void @unknown_assembly_variant() {
  ret void
}
