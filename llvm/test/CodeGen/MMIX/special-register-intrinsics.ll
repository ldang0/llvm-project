; RUN: llc -mtriple=mmix -O0 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=mmix -O0 -verify-machineinstrs -stop-after=mmix-isel < %s -o - | FileCheck %s --check-prefix=MIR
; RUN: llc -mtriple=mmix -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=OPT

target triple = "mmix"

; Writes that alter the MMIX C ABI or floating environment require a
; complete module-level assembly routine that owns the affected state.
module asm ".text"
module asm "mmix_special_state_owner:"
module asm "PUT rJ, r0"
module asm "PUT rG, r0"
module asm "PUT rL, r0"
module asm "PUT rA, r0"

; CHECK: mmix_special_state_owner:
; CHECK: PUT rJ, r0
; CHECK: PUT rG, r0
; CHECK: PUT rL, r0
; CHECK: PUT rA, r0

declare i64 @llvm.mmix.get(i32 immarg)
declare void @llvm.mmix.put(i32 immarg, i64)

define i64 @read_special_registers() {
; CHECK-LABEL: read_special_registers:
; CHECK:       GET [[B:r[0-9]+]], rB
; CHECK-NEXT:  GET [[ZZ:r[0-9]+]], rZZ
; CHECK-NEXT:  XOR r231, [[B]], [[ZZ]]
; MIR-LABEL:  name: read_special_registers
; MIR:        %{{[0-9]+}}:{{[^ ]+}} = GET $rb
; MIR-NEXT:   %{{[0-9]+}}:{{[^ ]+}} = GET $rzz
  %b = call i64 @llvm.mmix.get(i32 0)
  %zz = call i64 @llvm.mmix.get(i32 31)
  %result = xor i64 %b, %zz
  ret i64 %result
}

define void @retain_dead_read() {
; CHECK-LABEL: retain_dead_read:
; CHECK:       GET {{r[0-9]+}}, rC
; OPT-LABEL: retain_dead_read:
; OPT:       GET {{r[0-9]+}}, rC
  %unused = call i64 @llvm.mmix.get(i32 8)
  ret void
}

define i64 @read_same_register_twice() {
; CHECK-LABEL: read_same_register_twice:
; CHECK:       GET [[FIRST_B:r[0-9]+]], rB
; CHECK-NEXT:  GET [[SECOND_B:r[0-9]+]], rB
; OPT-LABEL: read_same_register_twice:
; OPT:       GET [[OPT_FIRST:r[0-9]+]], rB
; OPT-NEXT:  GET [[OPT_SECOND:r[0-9]+]], rB
  %first = call i64 @llvm.mmix.get(i32 0)
  %second = call i64 @llvm.mmix.get(i32 0)
  %result = xor i64 %first, %second
  ret i64 %result
}

define void @write_immediate() {
; CHECK-LABEL: write_immediate:
; CHECK:       PUT rH, 255
; MIR-LABEL:  name: write_immediate
; MIR:        PUT_RH_IMM 255, implicit-def dead $rh
  call void @llvm.mmix.put(i32 3, i64 255)
  ret void
}

define void @write_large_constant() {
; CHECK-LABEL: write_large_constant:
; CHECK:       SETL [[VALUE:r[0-9]+]], 256
; CHECK-NEXT:  PUT rH, [[VALUE]]
  call void @llvm.mmix.put(i32 3, i64 256)
  ret void
}

define void @write_register(i64 %value) {
; CHECK-LABEL: write_register:
; CHECK:       PUT rH, r231
; MIR-LABEL:  name: write_register
; MIR:        PUT_RH_REG %{{[0-9]+}}, implicit-def dead $rh
  call void @llvm.mmix.put(i32 3, i64 %value)
  ret void
}

define i64 @read_with_high_multiply(i64 %lhs, i64 %rhs) {
; CHECK-LABEL: read_with_high_multiply:
; CHECK:       MULU {{r[0-9]+}}, r231, r232
; CHECK-NEXT:  GET [[HIGH:r[0-9]+]], rH
; CHECK-NEXT:  GET [[FIRST:r[0-9]+]], rH
; CHECK:       GET [[SECOND:r[0-9]+]], rH
; MIR-LABEL:  name: read_with_high_multiply
; MIR:        %{{[0-9]+}}:{{[^ ]+}} = MULU {{.*}}implicit-def $rh
; MIR-NEXT:   %{{[0-9]+}}:{{[^ ]+}} = GET $rh
; MIR-NEXT:   %{{[0-9]+}}:{{[^ ]+}} = GET $rh
; MIR:        %{{[0-9]+}}:{{[^ ]+}} = GET $rh
  %before = call i64 @llvm.mmix.get(i32 3)
  %lhs.wide = zext i64 %lhs to i128
  %rhs.wide = zext i64 %rhs to i128
  %product = mul i128 %lhs.wide, %rhs.wide
  %high.wide = lshr i128 %product, 64
  %high = trunc i128 %high.wide to i64
  %after = call i64 @llvm.mmix.get(i32 3)
  %pair = xor i64 %before, %high
  %result = xor i64 %pair, %after
  ret i64 %result
}

define i64 @write_before_unsigned_divide(i64 %value, i64 %dividend,
                                         i64 %divisor) {
; CHECK-LABEL: write_before_unsigned_divide:
; CHECK:       PUT rD, r231
; CHECK-NEXT:  PUT rD, 0
; CHECK-NEXT:  DIVU r231, r232, r233
; MIR-LABEL:  name: write_before_unsigned_divide
; MIR:        PUT_RD_REG %{{[0-9]+}}, implicit-def dead $rd
; MIR-NEXT:   SET_RD_ZERO implicit-def $rd
; MIR-NEXT:   %{{[0-9]+}}:{{[^ ]+}} = DIVU {{.*}}implicit $rd
  call void @llvm.mmix.put(i32 1, i64 %value)
  %quotient = udiv i64 %dividend, %divisor
  ret i64 %quotient
}

define void @write_feature_state() {
; CHECK-LABEL: write_feature_state:
; CHECK:       PUT rC, 1
; CHECK-NEXT:  PUT rV, 2
  call void @llvm.mmix.put(i32 8, i64 1)
  call void @llvm.mmix.put(i32 18, i64 2)
  ret void
}

; Special-register words in labels and comments are not PUT instructions.
define void @non_instruction_special_register_words() {
; CHECK-LABEL: non_instruction_special_register_words:
; CHECK:       #APP
; CHECK-NEXT:  PUT:
; CHECK-NEXT:  # PUT rA, r0 is a comment
; CHECK-NEXT:  SWYM 0, 0, 0
; CHECK:       #NO_APP
  call void asm sideeffect "PUT:\0A\09# PUT rA, r0 is a comment\0A\09SWYM 0, 0, 0", ""()
  ret void
}
