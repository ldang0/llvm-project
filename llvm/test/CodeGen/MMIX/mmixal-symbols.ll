; RUN: llc -mtriple=mmix-unknown-elf -filetype=asm -O0 \
; RUN:   --output-asm-variant=1 %s -o - \
; RUN:   | FileCheck %s --implicit-check-not=.LBB \
; RUN:     --implicit-check-not='shared:' --implicit-check-not=.globl \
; RUN:     --implicit-check-not=.type --implicit-check-not=.size

target triple = "mmix-unknown-elf"

@named_data = internal global i64 7, align 8
@data_alias = internal alias i64, ptr @named_data
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

@function_alias = internal alias i64 (i1), ptr @shared

; The function and its entry block both claim "shared", so neither may use
; that spelling. Named non-entry blocks keep unique source aliases.
; CHECK:      __LLVM_U_6_736861726564	IS @
; CHECK-NEXT: __LLVM_B_F_736861726564_B_736861726564	IS @
; CHECK:      __LLVM_L_F_736861726564_BB_{{[0-9]+}}	IS @
; CHECK-NEXT: failure	IS @
; CHECK:      named_data	IS @
; CHECK:      __LLVM_L_F_736861726564_BB_{{[0-9]+}}	IS @
; CHECK-NEXT: success	IS @
; CHECK:      __LLVM_U_12_657363617065642E64617461	IS @
; CHECK:      data_alias	IS named_data
; CHECK-NEXT: function_alias	IS __LLVM_U_6_736861726564
