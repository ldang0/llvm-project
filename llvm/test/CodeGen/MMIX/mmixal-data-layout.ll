; RUN: llc -mtriple=mmix-unknown-elf -filetype=asm \
; RUN:   --output-asm-variant=1 %s -o - \
; RUN:   | FileCheck %s --implicit-check-not=: \
; RUN:     --implicit-check-not='{{^[[:space:]]*\.}}'

target triple = "mmix-unknown-elf"

@readonly_data = internal constant i64 72623859790382856, align 8
@writable_data = internal global i32 287454020, align 4
@data_pointer = internal global ptr @writable_data, align 8
@zero_data = internal global [16 x i8] zeroinitializer, align 16

define void @Main() {
entry:
  br label %loop

loop:
  br label %loop
}

define internal i64 @load_readonly() {
entry:
  %value = load volatile i64, ptr @readonly_data, align 8
  ret i64 %value
}

; Compound address materialization requires the read-only definition before
; the function body in MMIXAL source, while LOC preserves its planned address.
; CHECK:      __LLVM_L_F_6C6F61645F726561646F6E6C79_END_0	IS @
; CHECK:      LOC #2000000000000000
; CHECK-NEXT: readonly_data	IS @
; CHECK-NEXT: BYTE #01,#02,#03,#04,#05,#06,#07,#08
; CHECK:      LOC #000000000000010C
; CHECK-NEXT: load_readonly	IS @
; CHECK:      SETH ${{[0-9]+}},readonly_data>>48&65535
; CHECK-NEXT: INCMH ${{[0-9]+}},readonly_data>>32&65535
; CHECK-NEXT: INCML ${{[0-9]+}},readonly_data>>16&65535
; CHECK-NEXT: INCL ${{[0-9]+}},readonly_data&65535
; Natural alignment leaves address gaps without emitting synthetic data.
; CHECK:      LOC #2000000000000008
; CHECK:      writable_data	IS @
; CHECK-NEXT: BYTE #11,#22,#33,#44
; CHECK:      LOC #2000000000000010
; CHECK:      data_pointer	IS @
; CHECK-NEXT: OCTA writable_data
; CHECK:      LOC #2000000000000020
; CHECK:      zero_data	IS @
; CHECK-NEXT: OCTA 0,0
