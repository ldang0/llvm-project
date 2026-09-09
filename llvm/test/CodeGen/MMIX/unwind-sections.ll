; RUN: split-file %s %t
; RUN: llc -mtriple=mmix -filetype=obj %t/debug.ll -o %t/debug.o
; RUN: llvm-readobj --sections %t/debug.o | FileCheck %s --check-prefix=DEBUG --implicit-check-not=.eh_frame
; RUN: llvm-dwarfdump --debug-frame %t/debug.o | FileCheck %s --check-prefix=CFA
; RUN: llc -mtriple=mmix -filetype=obj %t/disabled.ll -o %t/disabled.o
; RUN: llvm-readobj --sections %t/disabled.o | FileCheck %s --check-prefix=DISABLED --implicit-check-not=.eh_frame --implicit-check-not=.debug_frame
; RUN: llc -mtriple=mmix -filetype=obj %t/default.ll -o %t/default.o
; RUN: llvm-readobj --sections %t/default.o | FileCheck %s --check-prefix=EH
; RUN: llvm-dwarfdump --eh-frame %t/default.o | FileCheck %s --check-prefix=RUNTIME
; RUN: llc -mtriple=mmix -filetype=obj %t/explicit.ll -o %t/explicit.o
; RUN: llvm-readobj --sections %t/explicit.o | FileCheck %s --check-prefix=EH
; DEBUG: Name: .debug_frame
; CFA: DW_CFA_def_cfa: R254 +0
; DISABLED: Name: .text
; EH: Name: .eh_frame
; RUNTIME: DW_CFA_offset: RJ -8

; A potentially throwing call needs runtime CFI without a personality or an
; explicit uwtable attribute. nounwind alone suppresses it, but uwtable wins.
;--- default.ll
declare void @callee()
define void @may_unwind() {
  call void @callee()
  ret void
}

;--- explicit.ll
define void @requested_table() nounwind uwtable(sync) {
  ret void
}

;--- debug.ll
define void @debug_leaf() nounwind !dbg !4 {
  ret void, !dbg !7
}
!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3}
!0 = distinct !DICompileUnit(language: DW_LANG_C11, file: !1, producer: "test", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug)
!1 = !DIFile(filename: "leaf.c", directory: "/")
!2 = !{i32 2, !"Dwarf Version", i32 4}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = distinct !DISubprogram(name: "debug_leaf", scope: !1, file: !1, line: 1, type: !5, scopeLine: 1, spFlags: DISPFlagDefinition, unit: !0)
!5 = !DISubroutineType(types: !6)
!6 = !{null}
!7 = !DILocation(line: 2, column: 1, scope: !4)

;--- disabled.ll
define void @plain_leaf() nounwind {
  ret void
}
