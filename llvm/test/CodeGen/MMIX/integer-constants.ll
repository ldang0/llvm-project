; RUN: llc -mtriple=mmix -verify-machineinstrs -filetype=asm %s -o - | FileCheck %s

target triple = "mmix"

; CHECK-LABEL: zero:
; CHECK:       SETL r231, 0
; CHECK-NEXT:  POP 0, 0
define i64 @zero() nounwind {
  ret i64 0
}

; CHECK-LABEL: low_wyde_max:
; CHECK:       SETL r231, 65535
; CHECK-NEXT:  POP 0, 0
define i64 @low_wyde_max() nounwind {
  ret i64 65535
}

; CHECK-LABEL: middle_low_wyde_max:
; CHECK:       SETML r231, 65535
; CHECK-NEXT:  POP 0, 0
define i64 @middle_low_wyde_max() nounwind {
  ret i64 4294901760
}

; CHECK-LABEL: middle_high_wyde_max:
; CHECK:       SETMH r231, 65535
; CHECK-NEXT:  POP 0, 0
define i64 @middle_high_wyde_max() nounwind {
  ret i64 281470681743360
}

; CHECK-LABEL: high_wyde_max:
; CHECK:       SETH r231, 65535
; CHECK-NEXT:  POP 0, 0
define i64 @high_wyde_max() nounwind {
  ret i64 -281474976710656
}

; CHECK-LABEL: signed_min:
; CHECK:       SETH r231, 32768
; CHECK-NEXT:  POP 0, 0
define i64 @signed_min() nounwind {
  ret i64 -9223372036854775808
}

; CHECK-LABEL: negative_one:
; CHECK:       NEGU r231, 0, 1
; CHECK-NEXT:  POP 0, 0
define i64 @negative_one() nounwind {
  ret i64 -1
}

; CHECK-LABEL: negative_255:
; CHECK:       NEGU r231, 0, 255
; CHECK-NEXT:  POP 0, 0
define i64 @negative_255() nounwind {
  ret i64 -255
}

; CHECK-LABEL: negative_256:
; CHECK:       SETL r231, 256
; CHECK-NEXT:  NEGU r231, 0, r231
; CHECK-NEXT:  POP 0, 0
define i64 @negative_256() nounwind {
  ret i64 -256
}

; CHECK-LABEL: negative_shifted_wyde:
; CHECK:       SETML r231, 1
; CHECK-NEXT:  NEGU r231, 0, r231
; CHECK-NEXT:  POP 0, 0
define i64 @negative_shifted_wyde() nounwind {
  ret i64 -65536
}

; CHECK-LABEL: complemented_shifted_wyde:
; CHECK:       SETML r231, 1
; CHECK-NEXT:  NOR r231, r231, 0
; CHECK-NEXT:  POP 0, 0
define i64 @complemented_shifted_wyde() nounwind {
  ret i64 -65537
}

; CHECK-LABEL: sparse_mixed_wydes:
; CHECK:       SETL r231, 22136
; CHECK-NEXT:  INCH r231, 4660
; CHECK-NEXT:  POP 0, 0
define i64 @sparse_mixed_wydes() nounwind {
  ret i64 1311673391471679096
}

; CHECK-LABEL: all_mixed_wydes:
; CHECK:       SETL r231, 57072
; CHECK-NEXT:  INCML r231, 39612
; CHECK-NEXT:  INCMH r231, 22136
; CHECK-NEXT:  INCH r231, 4660
; CHECK-NEXT:  POP 0, 0
define i64 @all_mixed_wydes() nounwind {
  ret i64 1311768467463790320
}
