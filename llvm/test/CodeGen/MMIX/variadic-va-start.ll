; RUN: llc -mtriple=mmix -verify-machineinstrs -stop-after=mmix-isel %s -o - \
; RUN:   | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=mmix -verify-machineinstrs -filetype=asm %s -o - \
; RUN:   | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=mmix -verify-machineinstrs -filetype=obj %s -o /dev/null

target triple = "mmix"

declare void @llvm.va_start(ptr)
declare void @sink(ptr)

; The final SP is S-136: eight bytes hold the local va_list object and the
; register-save area begins at final SP+8, or S-128. Every incoming argument
; register is saved in ascending order before va_start publishes that address.
; ISEL-LABEL: name: k0
; ISEL:       [[R246:%[0-9]+]]:{{[^ ]+}} = COPY $r246
; ISEL:       [[R231:%[0-9]+]]:{{[^ ]+}} = COPY $r231
; ISEL:       STOUI [[R231]], %fixed-stack.0, 0
; ISEL:       STOUI {{%[0-9]+}}, %fixed-stack.0, 8
; ISEL:       STOUI {{%[0-9]+}}, %fixed-stack.0, 16
; ISEL:       STOUI {{%[0-9]+}}, %fixed-stack.0, 24
; ISEL:       STOUI {{%[0-9]+}}, %fixed-stack.0, 32
; ISEL:       STOUI {{%[0-9]+}}, %fixed-stack.0, 40
; ISEL:       STOUI {{%[0-9]+}}, %fixed-stack.0, 48
; ISEL:       STOUI {{%[0-9]+}}, %fixed-stack.0, 56
; ISEL:       STOUI {{%[0-9]+}}, %fixed-stack.0, 64
; ISEL:       STOUI {{%[0-9]+}}, %fixed-stack.0, 72
; ISEL:       STOUI {{%[0-9]+}}, %fixed-stack.0, 80
; ISEL:       STOUI {{%[0-9]+}}, %fixed-stack.0, 88
; ISEL:       STOUI {{%[0-9]+}}, %fixed-stack.0, 96
; ISEL:       STOUI {{%[0-9]+}}, %fixed-stack.0, 104
; ISEL:       STOUI {{%[0-9]+}}, %fixed-stack.0, 112
; ISEL:       STOUI [[R246]], %fixed-stack.0, 120
; ISEL:       [[CURSOR:%[0-9]+]]:{{[^ ]+}} = ADDUI %fixed-stack.0, 0
; ISEL-NEXT:  STOUI [[CURSOR]], %stack.0.ap, 0
; ASM-LABEL: k0:
; ASM:       SUBU r254, r254, 136
; ASM-NEXT:  STOU r231, r254, 8
; ASM-NEXT:  STOU r232, r254, 16
; ASM-NEXT:  STOU r233, r254, 24
; ASM-NEXT:  STOU r234, r254, 32
; ASM-NEXT:  STOU r235, r254, 40
; ASM-NEXT:  STOU r236, r254, 48
; ASM-NEXT:  STOU r237, r254, 56
; ASM-NEXT:  STOU r238, r254, 64
; ASM-NEXT:  STOU r239, r254, 72
; ASM-NEXT:  STOU r240, r254, 80
; ASM-NEXT:  STOU r241, r254, 88
; ASM-NEXT:  STOU r242, r254, 96
; ASM-NEXT:  STOU r243, r254, 104
; ASM-NEXT:  STOU r244, r254, 112
; ASM-NEXT:  STOU r245, r254, 120
; ASM-NEXT:  STOU r246, r254, 128
; ASM-NEXT:  ADDU r231, r254, 8
; ASM-NEXT:  STOU r231, r254, 0
define ptr @k0(...) nounwind {
  %ap = alloca ptr, align 8
  call void @llvm.va_start(ptr %ap)
  %cursor = load ptr, ptr %ap, align 8
  ret ptr %cursor
}

; With K=15, $246 is saved at S-8 before va_start publishes S-8 and before
; the nested call. The forced frame pointer keeps both addresses stable.
; ISEL-LABEL: name: k15
; ISEL:       [[LAST:%[0-9]+]]:{{[^ ]+}} = COPY $r246
; ISEL-NEXT:  STOUI [[LAST]], %fixed-stack.0, 0
; ISEL-NEXT:  [[CURSOR:%[0-9]+]]:{{[^ ]+}} = ADDUI %fixed-stack.0, 0
; ISEL-NEXT:  STOUI [[CURSOR]], %stack.0.ap, 0
; ISEL:       DIRECT_CALL_STATE @sink
; ASM-LABEL: k15:
; ASM:       STOU r246, r253, {{r[0-9]+}}
; ASM:       ADDU r231, r253, {{r[0-9]+}}
; ASM:       STOU r231, r253, {{r[0-9]+}}
; ASM:       PUSHGO
define void @k15(i64, i64, i64, i64, i64, i64, i64, i64,
                 i64, i64, i64, i64, i64, i64, i64, ...) #0 {
  %ap = alloca ptr, align 8
  call void @llvm.va_start(ptr %ap)
  %cursor = load ptr, ptr %ap, align 8
  call void @sink(ptr %cursor)
  ret void
}

; K=16 has no unused argument registers. va_start publishes incoming S.
; ISEL-LABEL: name: k16
; ISEL:       liveins:         []
; ISEL:       body:
; ISEL-NOT:   STOUI {{.*}}, %fixed-stack.0
; ISEL:       [[CURSOR:%[0-9]+]]:{{[^ ]+}} = ADDUI %fixed-stack.0, 0
; ISEL-NEXT:  STOUI [[CURSOR]], %stack.0.ap, 0
; ASM-LABEL: k16:
; ASM:       SUBU r254, r254, 8
; ASM-NEXT:  ADDU r231, r254, 8
; ASM-NEXT:  STOU r231, r254, 0
define ptr @k16(i64, i64, i64, i64, i64, i64, i64, i64,
                i64, i64, i64, i64, i64, i64, i64, i64, ...) nounwind {
  %ap = alloca ptr, align 8
  call void @llvm.va_start(ptr %ap)
  %cursor = load ptr, ptr %ap, align 8
  ret ptr %cursor
}

; K=17 likewise emits no register save and advances the cursor to S+8.
; ISEL-LABEL: name: k17
; ISEL:       liveins:         []
; ISEL:       body:
; ISEL-NOT:   STOUI {{.*}}, %fixed-stack.1
; ISEL:       [[CURSOR:%[0-9]+]]:{{[^ ]+}} = ADDUI %fixed-stack.1, 0
; ISEL-NEXT:  STOUI [[CURSOR]], %stack.0.ap, 0
; ASM-LABEL: k17:
; ASM:       SUBU r254, r254, 8
; ASM-NEXT:  ADDU r231, r254, 16
; ASM-NEXT:  STOU r231, r254, 0
define ptr @k17(i64, i64, i64, i64, i64, i64, i64, i64,
                i64, i64, i64, i64, i64, i64, i64, i64, i64, ...) nounwind {
  %ap = alloca ptr, align 8
  call void @llvm.va_start(ptr %ap)
  %cursor = load ptr, ptr %ap, align 8
  ret ptr %cursor
}

attributes #0 = { "frame-pointer"="all" }
