; REQUIRES: mmix-registered-target
; RUN: llc -mtriple=mmix -O0 -filetype=obj %s -o %t.o
; RUN: ld.lld -m elf64mmix -e location_profile %t.o -o %t
; RUN: llvm-dwarfdump --verify %t
; RUN: llvm-dwarfdump --debug-info %t | FileCheck %s

; CHECK: DW_AT_name ("location_profile")
; CHECK: DW_AT_location
; CHECK-NEXT: {{.*}}DW_OP_reg7 R231)
; CHECK-NEXT: DW_AT_name ("register_arg")
; CHECK: DW_AT_location
; CHECK-NEXT: {{.*}}DW_OP_reg7 R231, DW_OP_piece 0x8, DW_OP_constu 0x2a, DW_OP_stack_value, DW_OP_piece 0x8
; CHECK: DW_AT_name ("piece_value")
; CHECK: DW_AT_const_value (42)
; CHECK: DW_AT_name ("constant_value")
; CHECK: DW_TAG_variable
; CHECK-NOT: DW_AT_location
; CHECK: DW_AT_name ("unavailable_value")
; CHECK: DW_AT_name ("stack_parameter")
; CHECK: DW_AT_location (DW_OP_fbreg +0)
; CHECK-NEXT: DW_AT_name ("stack_arg")

source_filename = "/src/unoptimized-locations.c"
target datalayout = "E-m:e-p:64:64-i64:64-n64-S64"
target triple = "mmix"

define i64 @location_profile(i64 %value) !dbg !5 {
entry:
  #dbg_value(i64 %value, !9, !DIExpression(), !14)
  #dbg_value(i64 42, !10, !DIExpression(), !14)
  #dbg_value(i64 poison, !11, !DIExpression(), !14)
  #dbg_value(i64 %value, !12, !DIExpression(DW_OP_LLVM_fragment, 0, 64), !14)
  #dbg_value(i64 42, !12, !DIExpression(DW_OP_LLVM_fragment, 64, 64), !14)
  %result = add i64 %value, 42, !dbg !15
  ret i64 %result, !dbg !16
}

define i64 @stack_parameter(i64 %a0, i64 %a1, i64 %a2, i64 %a3,
    i64 %a4, i64 %a5, i64 %a6, i64 %a7, i64 %a8, i64 %a9,
    i64 %a10, i64 %a11, i64 %a12, i64 %a13, i64 %a14, i64 %a15,
    i64 %stack_value) !dbg !19 {
entry:
  #dbg_value(i64 %stack_value, !22, !DIExpression(), !23)
  ret i64 %stack_value, !dbg !24
}

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3}
!llvm.ident = !{!4}

!0 = distinct !DICompileUnit(language: DW_LANG_C11, file: !1, producer: "MMIX location test", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "unoptimized-locations.c", directory: "/src")
!2 = !{i32 7, !"Dwarf Version", i32 5}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{!"MMIX location test"}
!5 = distinct !DISubprogram(name: "location_profile", scope: !1, file: !1, line: 1, type: !6, scopeLine: 1, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !8)
!6 = !DISubroutineType(types: !7)
!7 = !{!13, !13}
!8 = !{!9, !10, !11, !12}
!9 = !DILocalVariable(name: "register_arg", arg: 1, scope: !5, file: !1, line: 1, type: !13)
!10 = !DILocalVariable(name: "constant_value", scope: !5, file: !1, line: 2, type: !13)
!11 = !DILocalVariable(name: "unavailable_value", scope: !5, file: !1, line: 3, type: !13)
!12 = !DILocalVariable(name: "piece_value", scope: !5, file: !1, line: 4, type: !17)
!13 = !DIBasicType(name: "long", size: 64, encoding: DW_ATE_signed)
!14 = !DILocation(line: 5, column: 1, scope: !5)
!15 = !DILocation(line: 6, column: 1, scope: !5)
!16 = !DILocation(line: 7, column: 1, scope: !5)
!17 = !DICompositeType(tag: DW_TAG_structure_type, name: "pair", file: !1, line: 4, size: 128, elements: !18)
!18 = !{}
!19 = distinct !DISubprogram(name: "stack_parameter", scope: !1, file: !1, line: 10, type: !20, scopeLine: 10, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !21)
!20 = !DISubroutineType(types: !7)
!21 = !{!22}
!22 = !DILocalVariable(name: "stack_arg", arg: 17, scope: !19, file: !1, line: 10, type: !13)
!23 = !DILocation(line: 11, column: 1, scope: !19)
!24 = !DILocation(line: 12, column: 1, scope: !19)
