# RUN: llvm-mc -triple=mmix -filetype=obj %s -o %t.o
# RUN: llvm-readobj --sections --relocations %t.o | FileCheck %s --check-prefix=OBJ
# RUN: llvm-dwarfdump --eh-frame %t.o | FileCheck %s --check-prefix=CFI

# Explicit assembly CFI uses the same absolute pointer encoding as CodeGen.
.text
.cfi_sections .eh_frame
leaf:
.cfi_startproc
.cfi_def_cfa 30, 0
POP 0, 0
.cfi_endproc

# OBJ: Name: .eh_frame
# OBJ: SHF_ALLOC
# OBJ: AddressAlignment: 8
# OBJ: .rela.eh_frame {
# OBJ: R_MMIX_64 .text 0x0
# CFI: Augmentation: "zR"
# CFI: Return address column: 35
# CFI: Augmentation data: 00
# CFI: DW_CFA_def_cfa: R254 +0
