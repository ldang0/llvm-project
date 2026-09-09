; RUN: llc -mtriple=mmix -O0 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=mmix -O2 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=mmix -O0 -verify-machineinstrs -filetype=obj %s -o %t.o
; RUN: llc -mtriple=mmix -O0 -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefix=ISEL

; Both addresses must survive until the same asm. Reusing the reserved frame
; offset scratch register for the two operands would address the wrong slot.
define i64 @two_frame_operands() #0 {
; CHECK-LABEL: two_frame_operands:
; CHECK:       ADDU {{r[0-9]+}}, r253, r255
; CHECK:       ADDU {{r[0-9]+}}, r253, r255
; CHECK:       #APP
; CHECK:       LDO r255, {{r[0-9]+}}, 0
; CHECK:       LDO [[RESULT:r[0-9]+]], {{r[0-9]+}}, 0
; CHECK:       ADDU [[RESULT]], [[RESULT]], r255
; ISEL-LABEL: name: two_frame_operands
; ISEL-DAG:   [[FIRST:%[0-9]+]]:{{[a-z0-9]+}} = ADDUI %stack.0.a, 0
; ISEL-DAG:   [[SECOND:%[0-9]+]]:{{[a-z0-9]+}} = ADDUI %stack.1.b, 0
; ISEL:       INLINEASM {{.*}}mem:m, killed [[FIRST]], 0, mem:o, killed [[SECOND]], 0
  %a = alloca i64, align 8
  %b = alloca i64, align 8
  store i64 11, ptr %a
  store i64 31, ptr %b
  %sum = call i64 asm sideeffect
      "LDO r255, $1\0ALDO $0, $2\0AADDU $0, $0, r255",
      "=&r,*m,*o,~{r255}"(ptr elementtype(i64) %a, ptr elementtype(i64) %b)
  ret i64 %sum
}

define void @large_frame_address(i64 %value) #0 {
; CHECK-LABEL: large_frame_address:
; CHECK:       ADDU [[ADDRESS:r[0-9]+]], r253, r255
; CHECK:       #APP
; CHECK-NEXT:  STOU {{r[0-9]+}}, [[ADDRESS]], 0
  %buffer = alloca [512 x i8], align 8
  call void asm sideeffect "STOU $1, $0", "=*m,r"(
      ptr elementtype([512 x i8]) %buffer, i64 %value)
  ret void
}

define void @frame_address_constraint(i64 %value) #0 {
; CHECK-LABEL: frame_address_constraint:
; CHECK:       ADDU [[ADDRESS:r[0-9]+]], r253, r255
; CHECK:       #APP
; CHECK-NEXT:  STOU {{r[0-9]+}}, [[ADDRESS]], 0
  %slot = alloca i64, align 8
  call void asm sideeffect "STOU $1, $0", "p,r,~{memory}"(ptr %slot, i64 %value)
  ret void
}

define i64 @large_sp_offset() {
; CHECK-LABEL: large_sp_offset:
; CHECK:       ADDU [[ADDRESS:r[0-9]+]], r254, r255
; CHECK:       LDO {{r[0-9]+}}, [[ADDRESS]], 0
  %slot = alloca i64, align 8
  %padding = alloca [512 x i8], align 8
  store i64 42, ptr %slot
  call void asm sideeffect "", "*m"(ptr elementtype([512 x i8]) %padding)
  %result = call i64 asm sideeffect "LDO $0, $1", "=r,*m"(
      ptr elementtype(i64) %slot)
  ret i64 %result
}

attributes #0 = { "frame-pointer"="all" }
