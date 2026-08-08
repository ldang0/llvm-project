; RUN: llc -mtriple=mmix-unknown-elf -filetype=asm \
; RUN:   --output-asm-variant=1 %s -o - | FileCheck %s

target triple = "mmix-unknown-elf"

@entry_paths = internal constant [2 x ptr] [
  ptr blockaddress(@Main, %success),
  ptr blockaddress(@Main, %failure)
]

define void @Main() {
entry:
  %frame = alloca [64 x i64], align 8
  %slot = getelementptr inbounds [64 x i64], ptr %frame, i64 0, i64 63
  store volatile i64 40, ptr %slot, align 8
  %direct = call i64 @worker(ptr %slot)
  %use_leaf = icmp eq i64 %direct, 41
  %callee = select i1 %use_leaf, ptr @leaf, ptr @alternate
  %indirect = call i64 %callee(i64 %direct)
  %ok = icmp eq i64 %indirect, 42
  br i1 %ok, label %success, label %failure

success:
  store volatile i64 1, ptr %slot, align 8
  br label %loop

failure:
  store volatile i64 0, ptr %slot, align 8
  br label %loop

loop:
  br label %loop
}

define internal i64 @worker(ptr %slot) noinline {
entry:
  %local = alloca i64, align 8
  %input = load volatile i64, ptr %slot, align 8
  store volatile i64 %input, ptr %local, align 8
  %saved = load volatile i64, ptr %local, align 8
  %result = call i64 @leaf(i64 %saved)
  store volatile i64 %result, ptr %slot, align 8
  ret i64 %result
}

define internal i64 @leaf(i64 %value) noinline {
entry:
  %result = add i64 %value, 1
  ret i64 %result
}

define internal i64 @alternate(i64 %value) noinline {
entry:
  %result = sub i64 %value, 1
  ret i64 %result
}

; CHECK:      __LLVM_G_SP GREG #2000000004000000
; CHECK-NEXT: __LLVM_G_FP GREG 0
; CHECK-NEXT: __LLVM_G_R252 GREG 0
; CHECK:      __LLVM_G_R231 GREG 0

; Address-taken source blocks retain semantic aliases independent of layout
; and machine-basic-block numbering.
; CHECK:      [[SUCCESS:__LLVM_L_F_4D61696E_BA_[0-9]+]] IS @
; CHECK-NEXT: success IS @
; CHECK:      [[FAILURE:__LLVM_L_F_4D61696E_BA_[0-9]+]] IS @
; CHECK-NEXT: failure IS @
; CHECK:      loop IS @
; CHECK-NEXT: JMP [[LOOP:__LLVM_L_F_4D61696E_BB_[0-9]+]]

; The ordinary non-leaf callee owns an eight-byte frame, preserves its incoming
; rJ in the local-register window, restores SP and rJ, and returns with POP.
; CHECK:      worker IS @
; CHECK:      GET $30, rJ
; CHECK-NEXT: SUBU $254, $254, 8
; CHECK:      PUSHJ $31, leaf
; CHECK:      ADDU $254, $254, 8
; CHECK-NEXT: PUT rJ, $30
; CHECK-NEXT: POP 0, 0
; CHECK:      leaf IS @
; CHECK:      POP 0, 0
; CHECK:      alternate IS @
; CHECK:      POP 0, 0

; Main starts at the profile entry, defines the reserved expansion register
; before using it to allocate 512 bytes, and accesses the top slot at SP+504.
; Together with worker's frame, the tested low-water mark is stack-top-520.
; CHECK:      LOC #0000000000000100
; CHECK-NEXT: Main IS @
; CHECK-NEXT: PUT rA, 0
; CHECK-NEXT: PUT rL, 0
; CHECK-NEXT: GET $30, rJ
; CHECK-NEXT: SETL $255, 512
; CHECK-NEXT: SUBU $254, $254, $255
; CHECK-NEXT: SETL [[FRAME_OFFSET:\$[0-9]+]], 504
; CHECK-NEXT: ADDU [[FRAME_BASE:\$[0-9]+]], $254, 0
; CHECK:      STOU ${{[0-9]+}}, [[FRAME_BASE]], [[FRAME_OFFSET]]
; CHECK:      ADDU $231, [[FRAME_BASE]], [[FRAME_OFFSET]]
; CHECK-NEXT: PUSHJ $31, worker
; CHECK:      PUSHGO $31, ${{[0-9]+}}, 0
; CHECK-NOT:  POP
; CHECK:      entry_paths IS @
; CHECK-NEXT: OCTA [[SUCCESS]]
; CHECK-NEXT: OCTA [[FAILURE]]
