# REQUIRES: mmix-registered-target
# RUN: llvm-mc -triple=mmix -filetype=obj -dwarf-version=5 %s -o %t.o
# RUN: llvm-readobj --sections --relocations %t.o \
# RUN:   | FileCheck %s --check-prefix=RELOCS
# RUN: ld.lld -m elf64mmix -e dwarf_line_entry %t.o -o %t
# RUN: llvm-dwarfdump --verify %t
# RUN: llvm-dwarfdump --debug-line %t \
# RUN:   | FileCheck %s --check-prefix=LINE

# RELOCS-DAG: Name: .debug_line
# RELOCS-DAG: Name: .rela.debug_line
# RELOCS-DAG: Name: .debug_line_str
# RELOCS: Section ({{.*}}) .rela.debug_line {
# RELOCS: R_MMIX_32 .debug_line_str 0x0
# RELOCS: R_MMIX_32 .debug_line_str 0x{{[1-9A-F][0-9A-F]*}}
# RELOCS: R_MMIX_64 .text 0x0

# LINE: version: 5
# LINE: address_size: 8
# LINE: include_directories[  1] = "/src"
# LINE: name: "dwarf-line-relocations.s"
# LINE: {{0x[0-9a-f]+}} 5 1 1 {{.*}} is_stmt
# LINE-NEXT: {{0x[0-9a-f]+}} 7 1 1 {{.*}} is_stmt
# LINE-NEXT: {{0x[0-9a-f]+}} 7 1 1 {{.*}} is_stmt end_sequence

.text
.file 1 "/src" "dwarf-line-relocations.s"
.globl dwarf_line_entry
.type dwarf_line_entry,@function
dwarf_line_entry:
.loc 1 5 1
  SETL r0,42
.loc 1 7 1
  POP 1,0
.size dwarf_line_entry, .-dwarf_line_entry
