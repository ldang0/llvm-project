; RUN: llc -mtriple=mmix-unknown-elf -filetype=asm -asm-verbose=false %s \
; RUN:   -o %t.default
; RUN: llc -mtriple=mmix-unknown-elf -filetype=asm -asm-verbose=false \
; RUN:   --output-asm-variant=0 %s -o %t.explicit
; RUN: diff -u %t.default %t.explicit
; RUN: FileCheck %s < %t.default
;
; Keep the default assembly syntax tied to variant 0 when additional syntax
; variants are introduced. Match architectural spelling and ELF structure
; without depending on register allocation or temporary-label numbering.
;
; CHECK:      .text
; CHECK:      .globl canonical_output
; CHECK:      .type canonical_output,@function
; CHECK-LABEL: canonical_output:
; CHECK-NEXT: [[LOOP:\.LBB[0-9]+_[0-9]+]]:
; CHECK-NEXT: SUBU [[COUNTER:r[0-9]+]], [[COUNTER]], 1
; CHECK-NEXT: CMPU [[CONDITION:r[0-9]+]], [[COUNTER]], 0
; CHECK-NEXT: BNZB [[CONDITION]], [[LOOP]]
; CHECK:      GETA [[ADDRESS:r[0-9]+]], %geta(canonical_data)
; CHECK-NEXT: LDOU [[VALUE:r[0-9]+]], [[ADDRESS]], 0
; CHECK-NEXT: ADDU r{{[0-9]+}}, [[VALUE]], 7
; CHECK:      .size canonical_output, {{.*}}-canonical_output
; CHECK:      .type canonical_data,@object
; CHECK:      .data
; CHECK:      .globl canonical_data
; CHECK-LABEL: canonical_data:
; CHECK-NEXT: .8byte 42
; CHECK:      .size canonical_data, 8
; CHECK:      .section ".note.GNU-stack","",@progbits

target triple = "mmix-unknown-elf"

@canonical_data = global i64 42, align 8

define i64 @canonical_output(i64 %limit) nounwind {
entry:
  br label %loop

loop:
  %counter = phi i64 [ 0, %entry ], [ %next, %loop ]
  %next = add nuw i64 %counter, 1
  %done = icmp eq i64 %next, %limit
  br i1 %done, label %exit, label %loop

exit:
  %value = load volatile i64, ptr @canonical_data, align 8
  %result = add i64 %value, 7
  ret i64 %result
}
