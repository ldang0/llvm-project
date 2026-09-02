// REQUIRES: mmix-registered-target
// RUN: %clang --target=mmix -O2 -ffreestanding -fno-stack-protector \
// RUN:   -gdwarf-5 -g -ffile-prefix-map=%S=/src \
// RUN:   -fdebug-compilation-dir=/build -c %s -o %t.o
// RUN: llvm-readobj --sections %t.o | FileCheck %s --check-prefix=OBJECT
// RUN: ld.lld -m elf64mmix -e debug_entry %t.o -o %t
// RUN: llvm-dwarfdump --verify %t
// RUN: llvm-dwarfdump --debug-info %t \
// RUN:   | FileCheck %s --check-prefix=INFO

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
