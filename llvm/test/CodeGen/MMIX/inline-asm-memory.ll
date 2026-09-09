; RUN: llc -mtriple=mmix -O0 < %s | FileCheck %s

target triple = "mmix"

@object = global i64 0, align 8

define i64 @general_memory(ptr %p) {
; CHECK-LABEL: general_memory:
; CHECK:       #APP
; CHECK-NEXT:  LDO [[RESULT:r[0-9]+]], [[BASE:r[0-9]+]], 0
; CHECK:       #NO_APP
  %result = call i64 asm sideeffect "LDO $0, $1", "=r,*m"(
      ptr elementtype(i64) %p)
  ret i64 %result
}

define void @offsettable_stack(i64 %value) {
; CHECK-LABEL: offsettable_stack:
; CHECK:       ADDU [[ADDRESS:r[0-9]+]], r254, [[OFFSET:[0-9]+]]
; CHECK:       #APP
; CHECK-NEXT:  STOU [[VALUE:r[0-9]+]], [[ADDRESS]], 0
; CHECK:       #NO_APP
  %slot = alloca i64, align 8
  call void asm sideeffect "STOU $1, $0", "=*o,r"(
      ptr elementtype(i64) %slot, i64 %value)
  ret void
}

define i64 @general_symbolic_memory() {
; CHECK-LABEL: general_symbolic_memory:
; CHECK:       GETA [[ADDRESS:r[0-9]+]], %geta(object)
; CHECK:       #APP
; CHECK-NEXT:  LDO [[RESULT:r[0-9]+]], [[ADDRESS]], 0
; CHECK:       #NO_APP
  %result = call i64 asm sideeffect "LDO $0, $1", "=r,*m"(
      ptr elementtype(i64) @object)
  ret i64 %result
}

define i64 @register_offset(ptr %base, i64 %offset) {
; CHECK-LABEL: register_offset:
; CHECK:       #APP
; CHECK-NEXT:  LDO [[RESULT:r[0-9]+]], [[BASE:r[0-9]+]], [[OFFSET:r[0-9]+]]
; CHECK:       #NO_APP
  %address = getelementptr i8, ptr %base, i64 %offset
  %result = call i64 asm sideeffect "LDO $0, $1", "=r,*m"(
      ptr elementtype(i64) %address)
  ret i64 %result
}

define i64 @address_operand(ptr %p) {
; CHECK-LABEL: address_operand:
; CHECK:       #APP
; CHECK-NEXT:  LDO [[RESULT:r[0-9]+]], [[BASE:r[0-9]+]], 0
; CHECK:       #NO_APP
  %result = call i64 asm sideeffect "LDO $0, $1", "=r,p"(ptr %p)
  ret i64 %result
}

define ptr @non_offsettable_alternative(ptr %p) {
; CHECK-LABEL: non_offsettable_alternative:
; CHECK:       #APP
; CHECK-NEXT:  OR [[RESULT:r[0-9]+]], [[INPUT:r[0-9]+]], 0
; CHECK:       #NO_APP
  %result = call ptr asm sideeffect "OR $0, $1, 0", "=r,Vr"(ptr %p)
  ret ptr %result
}
