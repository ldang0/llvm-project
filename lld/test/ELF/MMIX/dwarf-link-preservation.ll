; REQUIRES: mmix
; RUN: split-file %s %t
; RUN: llc -mtriple=mmix -O2 -filetype=obj %t/input.ll -o %t/input.o
; RUN: llc -mtriple=mmix -O2 -filetype=obj %t/comdat.ll -o %t/comdat-a.o
; RUN: cp %t/comdat-a.o %t/comdat-b.o
; RUN: llvm-mc -triple=mmix -filetype=obj %t/bad-reloc.s -o %t/bad-reloc.o
; RUN: llvm-ar cr %t/input.a %t/input.o
; RUN: ld.lld -m elf64mmix -e dwarf_entry %t/input.o -o %t/direct
; RUN: ld.lld -m elf64mmix -r %t/input.o -o %t/partial.o
; RUN: ld.lld -m elf64mmix -r %t/partial.o -o %t/repeated.o
; RUN: ld.lld -m elf64mmix -e dwarf_entry %t/partial.o -o %t/staged
; RUN: ld.lld -m elf64mmix -e dwarf_entry %t/repeated.o -o %t/repeated
; RUN: ld.lld -m elf64mmix --gc-sections -e dwarf_entry %t/input.o -o %t/gc
; RUN: ld.lld -m elf64mmix -T %t/layout.lds -e dwarf_entry %t/input.o -o %t/script
; RUN: ld.lld -m elf64mmix -e dwarf_entry %t/input.a -o %t/archive
; RUN: ld.lld -m elf64mmix -e dwarf_entry %t/input.o %t/comdat-a.o \
; RUN:   %t/comdat-b.o -o %t/comdat
; RUN: ld.lld -m elf64mmix -e dwarf_entry %t/input.o %t/bad-reloc.o \
; RUN:   -o %t/bad-reloc
; RUN: llvm-readobj --relocations %t/bad-reloc \
; RUN:   | FileCheck %s --check-prefix=BAD-RELOC-ELF
; RUN: not llvm-dwarfdump --verify %t/bad-reloc 2>&1 \
; RUN:   | FileCheck %s --check-prefix=BAD-RELOC-DWARF
; RUN: llvm-dwarfdump --verify %t/direct
; RUN: llvm-dwarfdump --verify %t/partial.o
; RUN: llvm-dwarfdump --verify %t/repeated.o
; RUN: llvm-dwarfdump --verify %t/staged
; RUN: llvm-dwarfdump --verify %t/repeated
; RUN: llvm-dwarfdump --verify %t/gc
; RUN: llvm-dwarfdump --verify %t/script
; RUN: llvm-dwarfdump --verify %t/archive
; RUN: llvm-dwarfdump --verify %t/comdat
; RUN: llvm-nm --defined-only %t/comdat \
; RUN:   | FileCheck %s --check-prefix=COMDAT
; RUN: llvm-dwarfdump --debug-info --debug-loclists %t/direct \
; RUN:   | FileCheck %s --check-prefix=DWARF
; RUN: llvm-dwarfdump --debug-info --debug-loclists %t/staged \
; RUN:   | FileCheck %s --check-prefix=DWARF
; RUN: llvm-dwarfdump --debug-info --debug-loclists %t/repeated \
; RUN:   | FileCheck %s --check-prefix=DWARF
; RUN: llvm-dwarfdump --debug-info --debug-loclists %t/gc \
; RUN:   | FileCheck %s --check-prefix=DWARF
; RUN: llvm-dwarfdump --debug-info --debug-loclists %t/script \
; RUN:   | FileCheck %s --check-prefix=DWARF
; RUN: llvm-dwarfdump --debug-info --debug-loclists %t/archive \
; RUN:   | FileCheck %s --check-prefix=DWARF
; RUN: llvm-readobj --sections --relocations %t/partial.o \
; RUN:   | FileCheck %s --check-prefix=PARTIAL
; RUN: llvm-readobj --sections --relocations %t/repeated.o \
; RUN:   | FileCheck %s --check-prefix=PARTIAL
; RUN: llvm-objcopy --strip-debug %t/direct %t/stripped
; RUN: llvm-readobj --sections %t/stripped \
; RUN:   | FileCheck %s --check-prefix=STRIPPED --implicit-check-not=.debug_

; DWARF: DW_AT_name ("dwarf_entry")
; DWARF: DW_AT_location
; DWARF-NEXT: {{.*}}DW_OP_reg7 R231
; DWARF-NEXT: {{.*}}DW_OP_breg30 R254+0)
; DWARF-NEXT: DW_AT_name ("tracked")
; DWARF: .debug_loclists contents:

; PARTIAL-DAG: Name: .debug_info
; PARTIAL-DAG: Name: .debug_loclists
; PARTIAL-DAG: Name: .rela.debug_info
; PARTIAL: R_MMIX_64 .text

; STRIPPED: Name: .text
; COMDAT-COUNT-1: debug_comdat
; BAD-RELOC-ELF: Relocations [
; BAD-RELOC-ELF-NEXT: ]
; BAD-RELOC-DWARF: error: Unit Header Length:

;--- layout.lds
SECTIONS {
  . = 0x20000;
  .text : { *(.text .text.*) }
  .data : { *(.data .data.*) }
}

;--- input.ll
source_filename = "/src/dwarf-link.c"
target datalayout = "E-m:e-p:64:64-i64:64-n64-S64"
target triple = "mmix"

define i64 @dwarf_entry(i64 %value) !dbg !5 {
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
!0 = distinct !DICompileUnit(language: DW_LANG_C11, file: !1, producer: "MMIX lld DWARF test", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "dwarf-link.c", directory: "/src")
!2 = !{i32 7, !"Dwarf Version", i32 5}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{!"MMIX lld DWARF test"}
!5 = distinct !DISubprogram(name: "dwarf_entry", scope: !1, file: !1, line: 1, type: !6, scopeLine: 1, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !8)
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

;--- comdat.ll
source_filename = "/src/dwarf-comdat.c"
target datalayout = "E-m:e-p:64:64-i64:64-n64-S64"
target triple = "mmix"

$debug_comdat = comdat any

define linkonce_odr i64 @debug_comdat(i64 %value) comdat !dbg !5 {
entry:
  ret i64 %value, !dbg !10
}

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3}
!0 = distinct !DICompileUnit(language: DW_LANG_C11, file: !1, producer: "MMIX lld DWARF COMDAT test", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "dwarf-comdat.c", directory: "/src")
!2 = !{i32 7, !"Dwarf Version", i32 5}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!5 = distinct !DISubprogram(name: "debug_comdat", scope: !1, file: !1, line: 1, type: !6, scopeLine: 1, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !8)
!6 = !DISubroutineType(types: !7)
!7 = !{!9, !9}
!8 = !{}
!9 = !DIBasicType(name: "long", size: 64, encoding: DW_ATE_signed)
!10 = !DILocation(line: 2, column: 3, scope: !5)

;--- bad-reloc.s
.section .debug_info,"",@progbits
.quad missing_debug_target
