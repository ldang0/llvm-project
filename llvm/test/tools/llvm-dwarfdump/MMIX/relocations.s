# RUN: llvm-mc -triple=mmix -filetype=obj -dwarf-version=5 %s -o %t.o
# RUN: llvm-dwarfdump --verify %t.o
# RUN: llvm-dwarfdump --debug-line %t.o \
# RUN:   | FileCheck %s --check-prefix=LINE
# RUN: llvm-readobj --file-headers --sections --relocations %t.o \
# RUN:   | FileCheck %s --check-prefix=ELF
# RUN: llvm-objdump --section-headers --reloc %t.o \
# RUN:   | FileCheck %s --check-prefix=OBJDUMP

# LINE: file_names[  1]:
# LINE: name: "relocations.s"
# LINE: 0x0000000000000000 7 1 1 {{.*}} is_stmt
# LINE-NEXT: 0x0000000000000004 9 1 1 {{.*}} is_stmt

# ELF: Class: 64-bit
# ELF: DataEncoding: BigEndian
# ELF: Machine: EM_MMIX
# ELF-DAG: Name: .debug_line
# ELF: R_MMIX_32 .debug_line_str
# ELF: R_MMIX_64 .text

# OBJDUMP: .debug_line
# OBJDUMP: R_MMIX_32
# OBJDUMP: R_MMIX_64

.text
.file 1 "/src" "relocations.s"
.globl dwarf_relocation_entry
.type dwarf_relocation_entry,@function
dwarf_relocation_entry:
.loc 1 7 1
  SETL r0,42
.loc 1 9 1
  POP 1,0
.size dwarf_relocation_entry, .-dwarf_relocation_entry
