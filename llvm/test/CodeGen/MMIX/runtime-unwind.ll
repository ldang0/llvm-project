; RUN: llc -mtriple=mmix -filetype=asm %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=mmix -filetype=obj %s -o %t.o
; RUN: llvm-readobj --sections --relocations %t.o | FileCheck %s --check-prefix=OBJ
; RUN: llvm-dwarfdump --eh-frame %t.o | FileCheck %s --check-prefix=CFI
; RUN: llc -mtriple=mmix %s -o %t.s
; RUN: llvm-mc -triple=mmix -filetype=obj %t.s -o %t.roundtrip.o
; RUN: llvm-readobj --relocations %t.roundtrip.o | FileCheck %s --check-prefix=RELOC
; RUN: llc -O0 -mtriple=mmix -filetype=obj %s -o %t.o0.o
; RUN: llvm-dwarfdump --eh-frame %t.o0.o | FileCheck %s --check-prefix=CFI

; ASM-LABEL: leaf:
; ASM: .cfi_startproc
; ASM: .cfi_def_cfa r254, 0
; ASM: POP 0, 0
; ASM: .cfi_endproc
; ASM-LABEL: disabled:
; ASM-NOT: .cfi_startproc
; ASM: POP 0, 0
; ASM-NOT: .cfi_endproc

; OBJ: Name: .eh_frame
; OBJ: Type: SHT_PROGBITS
; OBJ: SHF_ALLOC
; OBJ: AddressAlignment: 8
; OBJ: .rela.eh_frame {
; OBJ: R_MMIX_64 .text 0x0
; OBJ-NOT: R_MMIX_32
; RELOC: .rela.eh_frame {
; RELOC: R_MMIX_64 .text 0x0
; CFI: .eh_frame contents:
; CFI: Version: 1
; CFI: Augmentation: "zR"
; CFI: Code alignment factor: 4
; CFI: Data alignment factor: -8
; CFI: Return address column: 35
; CFI: Augmentation data: 00
; CFI: DW_CFA_def_cfa: R254 +0

define void @leaf() nounwind uwtable(sync) {
  ret void
}

define void @disabled() nounwind {
  ret void
}
