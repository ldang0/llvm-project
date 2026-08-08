; RUN: llc -mtriple=mmix-unknown-elf -filetype=asm -O0 \
; RUN:   --output-asm-variant=1 %s -o - \
; RUN:   | FileCheck %s --implicit-check-not=: \
; RUN:     --implicit-check-not='{{^[[:space:]]*\.}}'

target triple = "mmix-unknown-elf"

@named_data = internal global i64 7, align 8
@data_alias_one = internal alias i64, ptr @named_data
@data_alias_two = internal alias i64, ptr @named_data
@"escaped.data" = internal global i8 1, align 1

define i64 @shared(i1 %condition) noinline optnone {
shared:
  br i1 %condition, label %success, label %failure

success:
  %value = load volatile i64, ptr @named_data, align 8
  ret i64 %value

failure:
  ret i64 0
}

@function_alias_one = internal alias i64 (i1), ptr @shared
@function_alias_two = internal alias i64 (i1), ptr @shared

; The function and its entry block both claim "shared", so neither may use
; that spelling. Named non-entry blocks keep unique source aliases.
; CHECK:      __LLVM_U_6_736861726564	IS @
; CHECK-NEXT: __LLVM_B_F_736861726564_B_736861726564	IS @
; CHECK:      __LLVM_L_F_736861726564_BB_{{[0-9]+}}	IS @
; CHECK-NEXT: failure	IS @
; CHECK:      __LLVM_L_F_736861726564_END_0	IS @
; CHECK:      named_data	IS @
; CHECK:      __LLVM_L_F_736861726564_BB_{{[0-9]+}}	IS @
; CHECK-NEXT: success	IS @
; CHECK:      __LLVM_U_12_657363617065642E64617461	IS @
; Multiple aliases at one data or function address emit no intervening bytes.
; CHECK:      data_alias_one	IS named_data
; CHECK-NEXT: data_alias_two	IS named_data
; CHECK-NEXT: function_alias_one	IS __LLVM_U_6_736861726564
; CHECK-NEXT: function_alias_two	IS __LLVM_U_6_736861726564
