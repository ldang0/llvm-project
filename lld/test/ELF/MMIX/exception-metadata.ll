; REQUIRES: mmix
; RUN: split-file %s %t
; RUN: llc -mtriple=mmix -exception-model=dwarf -function-sections -filetype=obj %t/input.ll -o %t/input.o
; RUN: llvm-readobj --section-groups --relocations %t/input.o | FileCheck %s --check-prefix=GROUP
; RUN: ld.lld --gc-sections -e live -T %t/layout.lds --defsym=base=0x10000 %t/input.o %t/input.o -o %t/low
; RUN: ld.lld --gc-sections -e live -T %t/layout.lds --defsym=base=0x8000000000010000 %t/input.o %t/input.o -o %t/high
; RUN: llvm-readobj --sections --symbols --relocations %t/low | FileCheck %s --check-prefix=LINK --implicit-check-not=unused_type --implicit-check-not=.gcc_except_table.dead
; RUN: llvm-readobj --sections --symbols --relocations %t/high | FileCheck %s --check-prefix=LINK --implicit-check-not=unused_type --implicit-check-not=.gcc_except_table.dead
; RUN: llvm-dwarfdump --eh-frame %t/low | FileCheck %s --check-prefixes=CFI,LOW
; RUN: llvm-dwarfdump --eh-frame %t/high | FileCheck %s --check-prefixes=CFI,HIGH
; RUN: llvm-readobj --hex-dump=.gcc_except_table.live %t/low | FileCheck %s --check-prefix=TYPE
; RUN: llvm-readobj --hex-dump=.gcc_except_table.live %t/high | FileCheck %s --check-prefix=TYPE
; RUN: llvm-as %t/input.ll -o %t/input.bc
; RUN: ld.lld --gc-sections -e live -T %t/layout.lds --defsym=base=0x10000 %t/input.bc -o %t/lto
; RUN: llvm-readobj --sections --symbols --relocations %t/lto | FileCheck %s --check-prefix=LINK --implicit-check-not=unused_type --implicit-check-not=.gcc_except_table.dead
; RUN: llvm-dwarfdump --eh-frame %t/lto | FileCheck %s --check-prefixes=CFI,LOW

; Duplicate COMDAT definitions must select their code and LSDA together.
; The undefined type referenced only by dead code must not retain its LSDA.
; Absolute stand-in symbols test relocation reach, not executable runtime code.
; Direct Full LTO uses the target's default DWARF exception model.
; GROUP: R_MMIX_64 typeinfo 0x0
; GROUP: R_MMIX_64 unused_type 0x0
; GROUP: R_MMIX_64 __gxx_personality_v0 0x0
; GROUP: R_MMIX_64 .gcc_except_table.live 0x0
; GROUP: R_MMIX_64 .gcc_except_table.dead 0x0
; GROUP: Signature: live
; GROUP: .text.live
; GROUP: .gcc_except_table.live
; GROUP: Signature: dead
; GROUP: .text.dead
; GROUP: .gcc_except_table.dead
; LINK: Name: .gcc_except_table.live
; LINK: SHF_ALLOC
; LINK: Name: .eh_frame
; LINK: SHF_ALLOC
; LINK: Relocations [
; LINK-NEXT: ]
; CFI: Augmentation: "zPLR"
; CFI: Personality Address: 1122334455667788
; LOW: pc=00010000...
; HIGH: pc=8000000000010000...
; LOW: LSDA Address: 0000000000020000
; HIGH: LSDA Address: 8000000000020000
; CFI-NOT: FDE cie=
; TYPE: 23456789
; TYPE-NEXT: {{0x[0-9a-f]+}} abcdef00

;--- layout.lds
SECTIONS {
  . = base;
  .text : { *(.text*) }
  . = base + 0x10000;
  .gcc_except_table.live : { *(.gcc_except_table.live) }
  .gcc_except_table.dead : { *(.gcc_except_table.dead) }
  .eh_frame : { *(.eh_frame) }
  __gxx_personality_v0 = 0x1122334455667788;
  typeinfo = 0x23456789abcdef00;
  callee = base + 0x8000;
}

;--- input.ll
target datalayout = "E-m:e-p:64:64-i64:64-n64-S64"
target triple = "mmix-unknown-unknown"

declare i32 @__gxx_personality_v0(...)
declare void @callee()
@typeinfo = external constant ptr
@unused_type = external constant ptr
$live = comdat any
$dead = comdat any

define linkonce_odr void @live() comdat personality ptr @__gxx_personality_v0 {
  invoke void @callee() to label %done unwind label %lpad
done:
  ret void
lpad:
  %lp = landingpad { ptr, i32 } catch ptr @typeinfo
  ret void
}

define linkonce_odr void @dead() comdat personality ptr @__gxx_personality_v0 {
  invoke void @callee() to label %done unwind label %lpad
done:
  ret void
lpad:
  %lp = landingpad { ptr, i32 } catch ptr @unused_type
  ret void
}
