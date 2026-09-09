; RUN: llc -mtriple=mmix -verify-machineinstrs -filetype=asm %s -o - | FileCheck %s

target triple = "mmix"

; Integer-to-floating conversions force IEEE round-to-nearest, independently
; of the current rA rounding mode.
; CHECK-LABEL: signed_to_double:
; CHECK:       FLOT r231, 4, r231
; CHECK-NEXT:  POP 0, 0
define double @signed_to_double(i64 %value) nounwind {
  %result = sitofp i64 %value to double
  ret double %result
}

; CHECK-LABEL: unsigned_to_double:
; CHECK:       FLOTU r231, 4, r231
; CHECK-NEXT:  POP 0, 0
define double @unsigned_to_double(i64 %value) nounwind {
  %result = uitofp i64 %value to double
  ret double %result
}

; Narrow integer operands are legalized with the source conversion's signed
; or unsigned extension before reaching FLOT.
; CHECK-LABEL: signed_i32_to_double:
; CHECK:       SLU [[EXT:r[0-9]+]], r231, 32
; CHECK-NEXT:  SR [[EXT]], [[EXT]], 32
; CHECK-NEXT:  FLOT r231, 4, [[EXT]]
define double @signed_i32_to_double(i32 %value) {
  %result = sitofp i32 %value to double
  ret double %result
}

; CHECK-LABEL: unsigned_i32_to_double:
; CHECK:       SETL [[MASK:r[0-9]+]], 65535
; CHECK-NEXT:  INCML [[MASK]], 65535
; CHECK-NEXT:  AND [[EXT:r[0-9]+]], r231, [[MASK]]
; CHECK-NEXT:  FLOTU r231, 4, [[EXT]]
define double @unsigned_i32_to_double(i32 %value) {
  %result = uitofp i32 %value to double
  ret double %result
}

; Constant conversions fold before instruction selection and materialize the
; resulting IEEE representation directly.
; CHECK-LABEL: signed_immediate_to_double:
; CHECK:       SETMH r231, 57344
; CHECK-NEXT:  INCH r231, 16495
; CHECK-NEXT:  POP 0, 0
define double @signed_immediate_to_double() nounwind {
  %result = sitofp i64 255 to double
  ret double %result
}

; Boundary constants follow the same exact bit-pattern path.
; CHECK-LABEL: signed_boundary_to_double:
; CHECK:       SETH r231, 50144
; CHECK-NEXT:  POP 0, 0
define double @signed_boundary_to_double() nounwind {
  %result = sitofp i64 -9223372036854775808 to double
  ret double %result
}

; FIXU avoids FIX's float-to-fix range trap. LLVM out-of-range results are
; poison and must not introduce a speculative architectural trap.
; CHECK-LABEL: double_to_signed:
; CHECK:       FIXU r231, 1, r231
; CHECK-NEXT:  POP 0, 0
define i64 @double_to_signed(double %value) nounwind {
  %result = fptosi double %value to i64
  ret i64 %result
}

; CHECK-LABEL: double_to_unsigned:
; CHECK:       FIXU r231, 1, r231
; CHECK-NEXT:  POP 0, 0
define i64 @double_to_unsigned(double %value) nounwind {
  %result = fptoui double %value to i64
  ret i64 %result
}

; CHECK-LABEL: double_to_signed_i32:
; CHECK:       FIXU
define i32 @double_to_signed_i32(double %value) {
  %result = fptosi double %value to i32
  ret i32 %result
}

; Floating octa loads and stores bridge the integer memory instruction's
; register class without changing the physical MMIX register.
; CHECK-LABEL: load_double:
; CHECK:       LDOU r231, r231, 0
define double @load_double(ptr %address) {
  %result = load double, ptr %address, align 8
  ret double %result
}

; CHECK-LABEL: store_double:
; CHECK:       STOU r232, r231, 0
define void @store_double(ptr %address, double %value) {
  store double %value, ptr %address, align 8
  ret void
}

; CHECK-LABEL: truncate_to_integral:
; CHECK:       FINT r231, 1, r231
define double @truncate_to_integral(double %value) {
  %result = call double @llvm.trunc.f64(double %value)
  ret double %result
}

; CHECK-LABEL: ceil_to_integral:
; CHECK:       FINT r231, 2, r231
define double @ceil_to_integral(double %value) {
  %result = call double @llvm.ceil.f64(double %value)
  ret double %result
}

; CHECK-LABEL: floor_to_integral:
; CHECK:       FINT r231, 3, r231
define double @floor_to_integral(double %value) {
  %result = call double @llvm.floor.f64(double %value)
  ret double %result
}

; CHECK-LABEL: round_even_to_integral:
; CHECK:       FINT r231, 4, r231
define double @round_even_to_integral(double %value) {
  %result = call double @llvm.roundeven.f64(double %value)
  ret double %result
}

; CHECK-LABEL: rint_to_integral:
; CHECK:       FINT r231, 4, r231
define double @rint_to_integral(double %value) {
  %result = call double @llvm.rint.f64(double %value)
  ret double %result
}

; Register-held f32 integral operations promote exactly to f64, use FINT, and
; round back to binary32 when the result is stored or returned.
; CHECK-LABEL: truncate_short_to_integral:
; CHECK:       FINT {{r[0-9]+}}, 1, {{r[0-9]+}}
define float @truncate_short_to_integral(float %value) {
  %result = call float @llvm.trunc.f32(float %value)
  ret float %result
}

; CHECK-LABEL: ceil_short_to_integral:
; CHECK:       FINT {{r[0-9]+}}, 2, {{r[0-9]+}}
define float @ceil_short_to_integral(float %value) {
  %result = call float @llvm.ceil.f32(float %value)
  ret float %result
}

; CHECK-LABEL: floor_short_to_integral:
; CHECK:       FINT {{r[0-9]+}}, 3, {{r[0-9]+}}
define float @floor_short_to_integral(float %value) {
  %result = call float @llvm.floor.f32(float %value)
  ret float %result
}

; CHECK-LABEL: round_even_short_to_integral:
; CHECK:       FINT {{r[0-9]+}}, 4, {{r[0-9]+}}
define float @round_even_short_to_integral(float %value) {
  %result = call float @llvm.roundeven.f32(float %value)
  ret float %result
}

; CHECK-LABEL: rint_short_to_integral:
; CHECK:       FINT {{r[0-9]+}}, 4, {{r[0-9]+}}
define float @rint_short_to_integral(float %value) {
  %result = call float @llvm.rint.f32(float %value)
  ret float %result
}

; f32 is memory-only: LDSF promotes it to the exact f64 register
; representation and STSF performs the required short-float rounding.
; CHECK-LABEL: load_short_float:
; CHECK:       LDSF r231, r231, 0
define double @load_short_float(ptr %address) {
  %short = load float, ptr %address, align 4
  %result = fpext float %short to double
  ret double %result
}

; CHECK-LABEL: load_short_float_offset:
; CHECK:       LDSF r231, r231, 12
define double @load_short_float_offset(ptr %address) {
  %element = getelementptr float, ptr %address, i64 3
  %short = load float, ptr %element, align 4
  %result = fpext float %short to double
  ret double %result
}

; CHECK-LABEL: load_short_float_register_offset:
; CHECK:       SLU [[OFFSET:r[0-9]+]], r232, 2
; CHECK:       LDSF r231, r231, [[OFFSET]]
define double @load_short_float_register_offset(ptr %address, i64 %index) {
  %element = getelementptr float, ptr %address, i64 %index
  %short = load float, ptr %element, align 4
  %result = fpext float %short to double
  ret double %result
}

; CHECK-LABEL: store_short_float:
; CHECK:       STSF r232, r231, 0
define void @store_short_float(ptr %address, double %value) {
  %short = fptrunc double %value to float
  store float %short, ptr %address, align 4
  ret void
}

; CHECK-LABEL: store_short_float_offset:
; CHECK:       STSF r232, r231, 20
define void @store_short_float_offset(ptr %address, double %value) {
  %element = getelementptr float, ptr %address, i64 5
  %short = fptrunc double %value to float
  store float %short, ptr %element, align 4
  ret void
}

; CHECK-LABEL: store_short_float_register_offset:
; CHECK:       SLU [[OFFSET:r[0-9]+]], r232, 2
; CHECK:       STSF r233, r231, [[OFFSET]]
define void @store_short_float_register_offset(ptr %address, i64 %index,
                                                double %value) {
  %element = getelementptr float, ptr %address, i64 %index
  %short = fptrunc double %value to float
  store float %short, ptr %element, align 4
  ret void
}

; Under-aligned short-float memory operations must not use LDSF or STSF,
; because MMIX rounds their addresses down to a four-byte boundary.
; CHECK-LABEL: copy_unaligned_short_float:
; CHECK-COUNT-4: LDBU
; CHECK-COUNT-4: STBU
; CHECK-NOT:   LDSF
; CHECK-NOT:   STSF
define void @copy_unaligned_short_float(ptr %out, ptr %in) {
  %value = load float, ptr %in, align 1
  store float %value, ptr %out, align 1
  ret void
}

; Direct integer-to-f32 conversion uses SFLOT so the register-held f64 value
; has already been rounded to short-float precision before STSF.
; CHECK-LABEL: signed_to_short_float:
; CHECK:       SFLOT [[SHORT:r[0-9]+]], 4, r232
; CHECK:       STSF [[SHORT]], r231, 0
define void @signed_to_short_float(ptr %address, i64 %value) {
  %short = sitofp i64 %value to float
  store float %short, ptr %address, align 4
  ret void
}

; CHECK-LABEL: unsigned_to_short_float:
; CHECK:       SFLOTU [[SHORT:r[0-9]+]], 4, r232
; CHECK:       STSF [[SHORT]], r231, 0
define void @unsigned_to_short_float(ptr %address, i64 %value) {
  %short = uitofp i64 %value to float
  store float %short, ptr %address, align 4
  ret void
}

; CHECK-LABEL: signed_i32_to_short_float:
; CHECK:       SLU [[EXT:r[0-9]+]], r232, 32
; CHECK-NEXT:  SR [[EXT]], [[EXT]], 32
; CHECK-NEXT:  SFLOT [[SHORT:r[0-9]+]], 4, [[EXT]]
; CHECK:       STSF [[SHORT]], r231, 0
define void @signed_i32_to_short_float(ptr %address, i32 %value) {
  %short = sitofp i32 %value to float
  store float %short, ptr %address, align 4
  ret void
}

; A reusable f32 result takes the same short-rounding path even without the
; store combine above.
; CHECK-LABEL: signed_to_short_float_value:
; CHECK:       SFLOT [[ROUNDED:r[0-9]+]], 4, r231
; CHECK:       STSF [[ROUNDED]], r254,
; CHECK:       LDTU r231, r254,
define float @signed_to_short_float_value(i64 %value) {
  %result = sitofp i64 %value to float
  ret float %result
}

; CHECK-LABEL: unsigned_to_short_float_value:
; CHECK:       SFLOTU [[ROUNDED:r[0-9]+]], 4, r231
; CHECK:       STSF [[ROUNDED]], r254,
; CHECK:       LDTU r231, r254,
define float @unsigned_to_short_float_value(i64 %value) {
  %result = uitofp i64 %value to float
  ret float %result
}

; Conversion from memory follows the explicit LDSF promotion path.
; CHECK-LABEL: short_float_to_signed:
; CHECK:       LDSF [[VALUE:r[0-9]+]], r231, 0
; CHECK:       FIXU r231, 1, [[VALUE]]
define i64 @short_float_to_signed(ptr %address) {
  %short = load float, ptr %address, align 4
  %value = fpext float %short to double
  %result = fptosi double %value to i64
  ret i64 %result
}

; A direct f32 conversion promotes the source numerically before FIXU.
; CHECK-LABEL: short_float_value_to_signed:
; CHECK:       STTU r231, r254,
; CHECK:       LDSF [[VALUE:r[0-9]+]], r254,
; CHECK:       FIXU r231, 1, [[VALUE]]
define i64 @short_float_value_to_signed(float %value) {
  %result = fptosi float %value to i64
  ret i64 %result
}

declare double @llvm.trunc.f64(double)
declare double @llvm.ceil.f64(double)
declare double @llvm.floor.f64(double)
declare double @llvm.roundeven.f64(double)
declare double @llvm.rint.f64(double)
declare float @llvm.trunc.f32(float)
declare float @llvm.ceil.f32(float)
declare float @llvm.floor.f32(float)
declare float @llvm.roundeven.f32(float)
declare float @llvm.rint.f32(float)
