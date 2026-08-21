; RUN: llc -mtriple=mmix-unknown-elf -filetype=asm -O0 \
; RUN:   --output-asm-variant=1 %s -o - | FileCheck %s

target triple = "mmix-unknown-elf"

%pair = type { i64, i64 }

; The buffered MMIXAL layout emits the callee before the caller even though
; their LLVM definitions have the opposite order. The callee initializes its
; own by-value object before writing the field.
; CHECK: aggregate_target	IS @
; CHECK: SUBU $254,$254,16
; CHECK: LDOU [[TARGET_HIGH:\$[0-9]+]],$231,8
; CHECK: STOU [[TARGET_HIGH]],$254,8
; CHECK: LDOU [[TARGET_LOW:\$[0-9]+]],$231,0
; CHECK: STOU [[TARGET_LOW]],$254,0
; CHECK: SETL [[VALUE:\$[0-9]+]],7
; CHECK: STOU [[VALUE]],$254,8
; CHECK: aggregate_owner	IS @
; CHECK: SUBU $254,$254,16
; CHECK: LDOU [[HIGH:\$[0-9]+]],$231,8
; CHECK: STOU [[HIGH]],$254,8
; CHECK: LDOU [[LOW:\$[0-9]+]],$231,0
; CHECK: STOU [[LOW]],$254,0
; CHECK: ADDU $231,$254,0
; CHECK: PUSHGO
define void @aggregate_owner(ptr %value) {
  call void @aggregate_target(ptr byval(%pair) align 8 %value)
  ret void
}

define void @Main() {
  br label %loop
loop:
  br label %loop
}

define void @aggregate_target(ptr byval(%pair) align 8 %value) {
  %field = getelementptr %pair, ptr %value, i64 0, i32 1
  store i64 7, ptr %field, align 8
  ret void
}
