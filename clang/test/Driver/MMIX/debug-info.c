// REQUIRES: mmix-registered-target
//
// RUN: %clang -### --target=mmix -g -c %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=FULL-DRIVER
// RUN: %clang -### --target=mmix-unknown-elf -gline-tables-only -c %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=LINE-DRIVER
//
// RUN: %clang --target=mmix -O0 -ffreestanding -fno-stack-protector \
// RUN:   -c %s -o %t.nodebug.o
// RUN: %clang --target=mmix -O0 -ffreestanding -fno-stack-protector \
// RUN:   -gdwarf-5 -g -ffile-prefix-map=%S=/src \
// RUN:   -fdebug-compilation-dir=/build -c %s -o %t.full.o
// RUN: llvm-readobj --sections --relocations %t.full.o \
// RUN:   | FileCheck %s --check-prefix=OBJECT
// RUN: ld.lld -m elf64mmix -e debug_entry %t.full.o -o %t.full
// RUN: llvm-dwarfdump --verify %t.full
// RUN: llvm-dwarfdump --debug-info %t.full \
// RUN:   | FileCheck %s --check-prefix=INFO
//
// RUN: %clang --target=mmix-unknown-elf -O2 -ffreestanding \
// RUN:   -fno-stack-protector -gdwarf-5 -gline-tables-only -c %s \
// RUN:   -o %t.line.o
// RUN: ld.lld -m elf64mmix -e debug_entry %t.line.o -o %t.line
// RUN: llvm-dwarfdump --verify %t.line

// FULL-DRIVER: "-triple" "mmix"
// FULL-DRIVER-SAME: "-debug-info-kind=constructor"
// FULL-DRIVER-SAME: "-dwarf-version=5"
// FULL-DRIVER-SAME: "-debugger-tuning=gdb"
// LINE-DRIVER: "-triple" "mmix-unknown-unknown"
// LINE-DRIVER-SAME: "-debug-info-kind=line-tables-only"
// LINE-DRIVER-SAME: "-dwarf-version=5"
// LINE-DRIVER-SAME: "-debugger-tuning=gdb"

// OBJECT-DAG: Name: .debug_info
// OBJECT-DAG: Name: .debug_abbrev
// OBJECT-DAG: Name: .debug_line
// OBJECT: Section ({{.*}}) .rela.debug_info {
// OBJECT: R_MMIX_{{32|64}}
// OBJECT: Section ({{.*}}) .rela.debug_line {
// OBJECT: R_MMIX_{{32|64}}

// INFO: version = 0x0005
// INFO: DW_TAG_compile_unit
// INFO: DW_AT_producer ("clang version {{.*}}")
// INFO: DW_AT_language (DW_LANG_C11)
// INFO: DW_AT_name ("/src/debug-info.c")
// INFO: DW_AT_comp_dir ("/build")
// INFO: DW_AT_low_pc
// INFO: DW_AT_high_pc
// INFO: DW_TAG_subprogram
// INFO: DW_AT_low_pc
// INFO: DW_AT_high_pc
// INFO: DW_AT_name ("debug_helper")
// INFO: DW_TAG_subprogram
// INFO: DW_AT_low_pc
// INFO: DW_AT_high_pc
// INFO: DW_AT_name ("debug_entry")
// INFO: DW_TAG_variable
// INFO: DW_AT_name ("debug_global")
// INFO: DW_TAG_base_type
// INFO: DW_AT_name ("int")

int debug_global = 7;

int debug_helper(int value) { return value + debug_global; }

int debug_entry(void) { return debug_helper(5); }
