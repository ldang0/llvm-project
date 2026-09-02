// REQUIRES: mmix-registered-target
// RUN: %clang --target=mmix -O2 -ffreestanding -fno-stack-protector \
// RUN:   -gdwarf-5 -g -ffile-prefix-map=%S=/src \
// RUN:   -fdebug-compilation-dir=/build -c %s -o %t.o
// RUN: llvm-readobj --sections %t.o | FileCheck %s --check-prefix=OBJECT
// RUN: ld.lld -m elf64mmix --image-base=0 -Ttext=0x1000 \
// RUN:   -e debug_entry %t.o -o %t
// RUN: llvm-dwarfdump --verify %t
// RUN: llvm-dwarfdump --debug-info %t \
// RUN:   | FileCheck %s --check-prefix=INFO
// RUN: llvm-symbolizer --inlines --obj=%t 0x1000 0x1004 0x100c 0x104c \
// RUN:   0xffffffffffffffff \
// RUN:   | FileCheck %s --check-prefix=SYMBOLIZE
// RUN: llvm-addr2line -f -i -e %t 0x1000 0x1004 0x100c 0x104c \
// RUN:   0xffffffffffffffff \
// RUN:   | FileCheck %s --check-prefix=ADDR2LINE

// OBJECT: Name: .debug_loclists

// INFO: DW_AT_name ("debug_helper")
// INFO: DW_AT_location
// INFO-NEXT: {{.*}}DW_OP_reg7 R231)
// INFO-NEXT: DW_AT_name ("value")
// INFO: DW_AT_location
// INFO-NEXT: {{.*}}DW_OP_reg26 R250)
// INFO-NEXT: DW_AT_name ("local")
// INFO: DW_TAG_inlined_subroutine
// INFO: DW_AT_abstract_origin{{.*}}"adjust"
// INFO: DW_TAG_formal_parameter
// INFO-NEXT: DW_AT_location (DW_OP_reg7 R231)
// INFO: DW_TAG_lexical_block
// INFO: DW_AT_location
// INFO-NEXT: {{.*}}DW_OP_breg26 R250+1, DW_OP_stack_value)
// INFO-NEXT: DW_AT_name ("scoped")
// INFO: DW_AT_name ("debug_entry")
// INFO: DW_AT_location
// INFO-NEXT: [{{.*}}, [[CALL_RETURN:0x[0-9a-f]+]]): DW_OP_reg7 R231)
// INFO-NEXT: DW_AT_name ("value")
// INFO: DW_AT_location
// INFO-NEXT: [{{.*}}, [[CALL_RETURN]]): {{.*}}DW_OP_mul, DW_OP_stack_value)
// INFO-NEXT: DW_AT_name ("unavailable_after_call")
// INFO: DW_AT_const_value (42)
// INFO-NEXT: DW_AT_name ("constant_value")
// INFO: DW_TAG_call_site
// INFO-NEXT: DW_AT_call_target_clobbered
// INFO-NEXT: DW_AT_call_return_pc ([[CALL_RETURN]])

// SYMBOLIZE: debug_helper
// SYMBOLIZE-NEXT: /src/optimized-debug-locations.c:{{[0-9]+}}:0
// SYMBOLIZE: debug_helper
// SYMBOLIZE-NEXT: /src/optimized-debug-locations.c:{{[0-9]+}}:0
// SYMBOLIZE: adjust
// SYMBOLIZE-NEXT: /src/optimized-debug-locations.c:{{[0-9]+}}:
// SYMBOLIZE-NEXT: debug_helper
// SYMBOLIZE-NEXT: /src/optimized-debug-locations.c:{{[0-9]+}}:
// SYMBOLIZE: debug_entry
// SYMBOLIZE-NEXT: /src/optimized-debug-locations.c:{{[0-9]+}}:0
// SYMBOLIZE: ??
// SYMBOLIZE-NEXT: ??:0:0

// ADDR2LINE: debug_helper
// ADDR2LINE-NEXT: /src/optimized-debug-locations.c:{{[0-9]+}}
// ADDR2LINE: debug_helper
// ADDR2LINE-NEXT: /src/optimized-debug-locations.c:{{[0-9]+}}
// ADDR2LINE: adjust
// ADDR2LINE-NEXT: /src/optimized-debug-locations.c:{{[0-9]+}}
// ADDR2LINE-NEXT: debug_helper
// ADDR2LINE-NEXT: /src/optimized-debug-locations.c:{{[0-9]+}}
// ADDR2LINE: debug_entry
// ADDR2LINE-NEXT: /src/optimized-debug-locations.c:{{[0-9]+}}
// ADDR2LINE: ??
// ADDR2LINE-NEXT: ??:0

int debug_global = 9;

static __attribute__((always_inline)) inline int adjust(int value) {
  return value + debug_global;
}

__attribute__((noinline)) int debug_helper(int value) {
  int local = adjust(value);
  {
    int scoped = local + 1;
    if (value < 0)
      return scoped;
  }
  return local * 2;
}

int debug_entry(int value) {
  int constant_value = 42;
  int unavailable_after_call = value * 7;
  return debug_helper(value) + constant_value;
}
