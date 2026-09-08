; RUN: llc -mtriple=mmix -exception-model=dwarf %s -o %t.s
; RUN: FileCheck %s --check-prefix=ASM < %t.s
; RUN: llc -O0 -mtriple=mmix -exception-model=dwarf %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=mmix -exception-model=dwarf -filetype=obj %s -o %t.o
; RUN: llvm-mc -triple=mmix -filetype=obj %t.s -o %t.mc.o
; RUN: llvm-readobj -r %t.o | FileCheck %s --check-prefix=OBJ
; RUN: llvm-readobj -r %t.mc.o | FileCheck %s --check-prefix=OBJ
; RUN: llvm-dwarfdump --eh-frame %t.o | FileCheck %s --check-prefix=CFI
; RUN: llvm-dwarfdump --eh-frame %t.mc.o | FileCheck %s --check-prefix=CFI

declare i32 @__gxx_personality_v0(...)
declare void @callee()
declare void @consume(ptr, i32) nounwind
@typeinfo = external constant ptr

; ASM-LABEL: cleanup:
; ASM: .cfi_personality 0, __gxx_personality_v0
; ASM: .cfi_lsda 0, [[CLEAN:.Lexception[0-9]+]]
; ASM: [[BEGIN:.Ltmp[0-9]+]]:
; ASM: PUSHGO
; ASM: [[END:.Ltmp[0-9]+]]:
; ASM: [[PAD:.Ltmp[0-9]+]]:
; ASM: [[CLEAN]]:
; ASM-NEXT: .byte 255 {{.*}}LPStart Encoding = omit
; ASM-NEXT: .byte 255 {{.*}}TType Encoding = omit
; ASM-NEXT: .byte 1 {{.*}}Call site Encoding = uleb128
; ASM: .uleb128 [[BEGIN]]-.Lfunc_begin0
; ASM-NEXT: .uleb128 [[END]]-[[BEGIN]]
; ASM-NEXT: .uleb128 [[PAD]]-.Lfunc_begin0
; ASM-NEXT: .byte 0 {{.*}}On action: cleanup
define void @cleanup() personality ptr @__gxx_personality_v0 {
  invoke void @callee() to label %done unwind label %lpad
done:
  ret void
lpad:
  %lp = landingpad { ptr, i32 } cleanup
  resume { ptr, i32 } %lp
}

; ASM-LABEL: handlers:
; ASM: .cfi_personality 0, __gxx_personality_v0
; ASM: .cfi_lsda 0, [[HANDLERS:.Lexception[0-9]+]]
; ASM: [[HANDLERS]]:
; ASM-NEXT: .byte 255 {{.*}}LPStart Encoding = omit
; ASM-NEXT: .byte 0 {{.*}}TType Encoding = absptr
; ASM: .byte 1 {{.*}}Call site Encoding = uleb128
; The chain tests the typed catch before the terminating catch-all.
; ASM: .byte 3 {{.*}}On action: 2
; ASM: .byte 1 {{.*}}Action Record 1
; ASM-NEXT: {{.*}}Catch TypeInfo 1
; ASM-NEXT: .byte 0 {{.*}}No further actions
; ASM: .byte 2 {{.*}}Action Record 2
; ASM-NEXT: {{.*}}Catch TypeInfo 2
; ASM-NEXT: .byte 125 {{.*}}Continue to action 1
; ASM: .8byte typeinfo {{.*}}TypeInfo 2
; ASM-NEXT: .8byte 0 {{.*}}TypeInfo 1
define void @handlers() personality ptr @__gxx_personality_v0 {
  invoke void @callee() to label %done unwind label %lpad
done:
  ret void
lpad:
  %lp = landingpad { ptr, i32 } catch ptr @typeinfo catch ptr null
  %ptr = extractvalue { ptr, i32 } %lp, 0
  %selector = extractvalue { ptr, i32 } %lp, 1
  call void @consume(ptr %ptr, i32 %selector)
  ret void
}

; OBJ: .rela.gcc_except_table {
; OBJ-NEXT: {{.*}} R_MMIX_64 typeinfo 0x0
; OBJ-NEXT: }
; OBJ: .rela.eh_frame {
; OBJ: R_MMIX_64 __gxx_personality_v0 0x0
; OBJ: R_MMIX_64 .text
; OBJ: R_MMIX_64 .gcc_except_table 0x0
; OBJ: R_MMIX_64 .text
; OBJ: R_MMIX_64 .gcc_except_table
; CFI: Augmentation: "zPLR"
; CFI: Return address column: 35
; CFI: Augmentation data: 00 00 00 00 00 00 00 00 00 00 00
; CFI: FDE cie=
; CFI: LSDA Address:
; CFI: FDE cie=
; CFI: LSDA Address:
