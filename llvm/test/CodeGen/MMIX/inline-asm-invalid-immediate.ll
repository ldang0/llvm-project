; RUN: not llc -mtriple=mmix -O0 < %s -o /dev/null 2>&1 | FileCheck %s

target triple = "mmix"

define void @invalid_i() {
; CHECK: error: value out of range for constraint 'I'
  call void asm sideeffect "SWYM $0, 0, 0", "I"(i64 256)
  ret void
}

define void @invalid_j() {
; CHECK: error: value out of range for constraint 'J'
  call void asm sideeffect "SWYM 0, 0, 0", "J"(i64 65536)
  ret void
}

define void @invalid_k() {
; CHECK: error: value out of range for constraint 'K'
  call void asm sideeffect "SWYM 0, 0, 0", "K"(i64 -256)
  ret void
}

define void @invalid_m() {
; CHECK: error: value out of range for constraint 'M'
  call void asm sideeffect "SWYM 0, 0, 0", "M"(i64 1)
  ret void
}

define void @invalid_o() {
; CHECK: error: value out of range for constraint 'O'
  call void asm sideeffect "SWYM 0, 0, 0", "O"(i64 4)
  ret void
}

define void @invalid_g() {
; CHECK: error: invalid operand for inline asm constraint 'G'
  call void asm sideeffect "SWYM 0, 0, 0", "G"(double 1.0)
  ret void
}
