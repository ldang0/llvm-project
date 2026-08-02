; RUN: llc -mtriple=mmix -verify-machineinstrs -filetype=asm %s -o - | FileCheck %s

target triple = "mmix"

define i64 @load_sext_i8(ptr %p) {
; CHECK-LABEL: load_sext_i8:
; CHECK: LDB r231, r231, 0
  %v = load i8, ptr %p, align 1
  %r = sext i8 %v to i64
  ret i64 %r
}

define i64 @load_zext_i8(ptr %p) {
; CHECK-LABEL: load_zext_i8:
; CHECK: LDBU r231, r231, 0
  %v = load i8, ptr %p, align 1
  %r = zext i8 %v to i64
  ret i64 %r
}

define i64 @load_sext_i16(ptr %p) {
; CHECK-LABEL: load_sext_i16:
; CHECK: LDW r231, r231, 0
  %v = load i16, ptr %p, align 2
  %r = sext i16 %v to i64
  ret i64 %r
}

define i64 @load_zext_i16(ptr %p) {
; CHECK-LABEL: load_zext_i16:
; CHECK: LDWU r231, r231, 0
  %v = load i16, ptr %p, align 2
  %r = zext i16 %v to i64
  ret i64 %r
}

define i64 @load_sext_i32(ptr %p) {
; CHECK-LABEL: load_sext_i32:
; CHECK: LDT r231, r231, 0
  %v = load i32, ptr %p, align 4
  %r = sext i32 %v to i64
  ret i64 %r
}

define i64 @load_zext_i32(ptr %p) {
; CHECK-LABEL: load_zext_i32:
; CHECK: LDTU r231, r231, 0
  %v = load i32, ptr %p, align 4
  %r = zext i32 %v to i64
  ret i64 %r
}

define i64 @load_i64(ptr %p) {
; CHECK-LABEL: load_i64:
; CHECK: LDOU r231, r231, 0
  %v = load i64, ptr %p, align 8
  ret i64 %v
}

define void @store_widths(ptr %p8, ptr %p16, ptr %p32, ptr %p64, i64 %v) {
; CHECK-LABEL: store_widths:
; CHECK: STBU r235, r231, 0
; CHECK: STWU r235, r232, 0
; CHECK: STTU r235, r233, 0
; CHECK: STOU r235, r234, 0
  %v8 = trunc i64 %v to i8
  store i8 %v8, ptr %p8, align 1
  %v16 = trunc i64 %v to i16
  store i16 %v16, ptr %p16, align 2
  %v32 = trunc i64 %v to i32
  store i32 %v32, ptr %p32, align 4
  store i64 %v, ptr %p64, align 8
  ret void
}

define i64 @load_small_offset(ptr %p) {
; CHECK-LABEL: load_small_offset:
; CHECK: LDOU r231, r231, 248
  %q = getelementptr i8, ptr %p, i64 248
  %v = load i64, ptr %q, align 8
  ret i64 %v
}

define i64 @load_large_offset(ptr %p) {
; CHECK-LABEL: load_large_offset:
; CHECK: SETL [[OFFSET:r[0-9]+]], 256
; CHECK: LDBU r231, r231, [[OFFSET]]
  %q = getelementptr i8, ptr %p, i64 256
  %v = load i8, ptr %q, align 1
  %r = zext i8 %v to i64
  ret i64 %r
}

define i64 @load_register_offset(ptr %p, i64 %offset) {
; CHECK-LABEL: load_register_offset:
; CHECK: LDBU r231, r231, r232
  %q = getelementptr i8, ptr %p, i64 %offset
  %v = load i8, ptr %q, align 1
  %r = zext i8 %v to i64
  ret i64 %r
}

define void @store_small_offset(ptr %p, i64 %v) {
; CHECK-LABEL: store_small_offset:
; CHECK: STTU r232, r231, 16
  %q = getelementptr i8, ptr %p, i64 16
  %v32 = trunc i64 %v to i32
  store i32 %v32, ptr %q, align 4
  ret void
}

define void @store_register_offset(ptr %p, i64 %offset, i64 %v) {
; CHECK-LABEL: store_register_offset:
; CHECK: STWU r233, r231, r232
  %q = getelementptr i8, ptr %p, i64 %offset
  %v16 = trunc i64 %v to i16
  store i16 %v16, ptr %q, align 2
  ret void
}

define i64 @load_scaled_index(ptr %p, i64 %index) {
; CHECK-LABEL: load_scaled_index:
; CHECK: SLU [[OFFSET:r[0-9]+]], r232, 3
; CHECK: LDOU r231, r231, [[OFFSET]]
  %q = getelementptr i64, ptr %p, i64 %index
  %v = load i64, ptr %q, align 8
  ret i64 %v
}

define i64 @stack_object() {
; CHECK-LABEL: stack_object:
; CHECK: STOU {{r[0-9]+}}, r254, 0
; CHECK: LDOU r231, r254, 0
  %slot = alloca i64, align 8
  store volatile i64 42, ptr %slot, align 8
  %r = load volatile i64, ptr %slot, align 8
  ret i64 %r
}

define i64 @load_unaligned_i64(ptr %p) {
; CHECK-LABEL: load_unaligned_i64:
; CHECK-COUNT-8: LDBU
; CHECK-NOT: LDOU
  %v = load i64, ptr %p, align 1
  ret i64 %v
}

define i64 @load_unaligned_i16(ptr %p) {
; CHECK-LABEL: load_unaligned_i16:
; CHECK-COUNT-2: LDBU
; CHECK-NOT: LDWU
  %v = load i16, ptr %p, align 1
  %r = zext i16 %v to i64
  ret i64 %r
}

define void @store_unaligned_i64(ptr %p, i64 %v) {
; CHECK-LABEL: store_unaligned_i64:
; CHECK-COUNT-8: STBU
; CHECK-NOT: STOU
  store i64 %v, ptr %p, align 1
  ret void
}
