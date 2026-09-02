; REQUIRES: mmix-registered-target
; RUN: llc -mtriple=mmix -O2 -filetype=obj %s -o %t.o
; RUN: ld.lld -m elf64mmix -e location_transition %t.o -o %t
; RUN: llvm-dwarfdump --verify %t
; RUN: llvm-dwarfdump --debug-info %t | FileCheck %s

; CHECK: DW_AT_name ("location_transition")
; CHECK: DW_AT_location
; CHECK-NEXT: [{{.*}}, [[STACK:0x[0-9a-f]+]]): DW_OP_reg7 R231
; CHECK-NEXT: [[[STACK]], {{.*}}): DW_OP_breg30 R254+0)
; CHECK-NEXT: DW_AT_name ("tracked")

source_filename = "/src/optimized-locations.c"
target datalayout = "E-m:e-p:64:64-i64:64-n64-S64"
target triple = "mmix"

define i64 @location_transition(i64 %value) !dbg !5 {
entry:
  %slot = alloca i64, align 8
  #dbg_value(i64 %value, !9, !DIExpression(), !10)
  store volatile i64 %value, ptr %slot, align 8, !dbg !11
  #dbg_value(ptr %slot, !9, !DIExpression(DW_OP_deref), !12)
  %result = load volatile i64, ptr %slot, align 8, !dbg !13
  ret i64 %result, !dbg !14
}

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3}
!llvm.ident = !{!4}

!0 = distinct !DICompileUnit(language: DW_LANG_C11, file: !1, producer: "MMIX optimized location test", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "optimized-locations.c", directory: "/src")
!2 = !{i32 7, !"Dwarf Version", i32 5}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{!"MMIX optimized location test"}
!5 = distinct !DISubprogram(name: "location_transition", scope: !1, file: !1, line: 1, type: !6, scopeLine: 1, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !8)
!6 = !DISubroutineType(types: !7)
!7 = !{!15, !15}
!8 = !{!9}
!9 = !DILocalVariable(name: "tracked", arg: 1, scope: !5, file: !1, line: 1, type: !15)
!10 = !DILocation(line: 1, column: 1, scope: !5)
!11 = !DILocation(line: 2, column: 1, scope: !5)
!12 = !DILocation(line: 3, column: 1, scope: !5)
!13 = !DILocation(line: 4, column: 1, scope: !5)
!14 = !DILocation(line: 5, column: 1, scope: !5)
!15 = !DIBasicType(name: "long", size: 64, encoding: DW_ATE_signed)
