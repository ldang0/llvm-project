// REQUIRES: mmix-registered-target
// RUN: %clang --target=mmix -O0 -ffreestanding -fno-stack-protector \
// RUN:   -gdwarf-5 -g -ffile-prefix-map=%S=/src \
// RUN:   -fdebug-compilation-dir=/build -c %s -o %t.o0.o
// RUN: llvm-readobj --relocations %t.o0.o \
// RUN:   | FileCheck %s --check-prefix=OBJECT
// RUN: ld.lld -m elf64mmix -e debug_line_entry %t.o0.o -o %t.o0
// RUN: llvm-dwarfdump --verify %t.o0
// RUN: llvm-dwarfdump --debug-line %t.o0 \
// RUN:   | FileCheck %s --check-prefixes=COMMON,O0
//
// RUN: %clang --target=mmix-unknown-elf -O2 -ffreestanding \
// RUN:   -fno-stack-protector -gdwarf-5 -gline-tables-only \
// RUN:   -ffile-prefix-map=%S=/src -fdebug-compilation-dir=/build \
// RUN:   -c %s -o %t.o2.o
// RUN: llvm-readobj --relocations %t.o2.o \
// RUN:   | FileCheck %s --check-prefix=OBJECT
// RUN: ld.lld -m elf64mmix -e debug_line_entry %t.o2.o -o %t.o2
// RUN: llvm-dwarfdump --verify %t.o2
// RUN: llvm-dwarfdump --debug-line %t.o2 \
// RUN:   | FileCheck %s --check-prefixes=COMMON,O2

// OBJECT: Section ({{.*}}) .rela.debug_line {
// OBJECT: R_MMIX_32 .debug_line_str 0x0
// OBJECT: R_MMIX_64 .text 0x0

// COMMON: version: 5
// COMMON: address_size: 8
// COMMON: include_directories[  0] = "/build"
// COMMON: name: "/src/debug-line.c"
// O0: {{0x[0-9a-f]+}} 101 {{.*}} is_stmt
// O0: {{0x[0-9a-f]+}} 102 {{.*}} is_stmt
// O0: {{0x[0-9a-f]+}} 103 {{.*}} is_stmt
// O0: {{0x[0-9a-f]+}} 104 {{.*}} is_stmt
// O0: {{0x[0-9a-f]+}} 199 {{.*}} is_stmt
// O0: {{0x[0-9a-f]+}} 200 {{.*}} is_stmt
// O0: {{0x[0-9a-f]+}} 201 {{.*}} is_stmt
// O2: {{0x[0-9a-f]+}} 101 {{.*}} is_stmt
// O2: {{0x[0-9a-f]+}} 102 {{.*}} is_stmt
// O2: {{0x[0-9a-f]+}} 103 {{.*}} is_stmt
// O2: {{0x[0-9a-f]+}} 104 {{.*}} is_stmt
// O2: {{0x[0-9a-f]+}} 199 {{.*}} is_stmt
// O2: {{0x[0-9a-f]+}} 200 {{.*}} is_stmt
// O2: {{0x[0-9a-f]+}} 201 {{.*}} is_stmt

#line 99 "/src/debug-line.c"
volatile int debug_line_global = 9;

__attribute__((noinline)) int debug_line_helper(int value) {
  int loaded = debug_line_global;
  int adjusted = value + loaded;
  return adjusted * 2;
}

#line 199 "/src/debug-line.c"
int debug_line_entry(void) {
  int input = debug_line_global;
  return debug_line_helper(input) + 1;
}
