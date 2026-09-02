// REQUIRES: mmix-registered-target
// RUN: %clang --target=mmix -O0 -ffreestanding -fno-stack-protector \
// RUN:   -gdwarf-5 -g -ffile-prefix-map=%S=/src \
// RUN:   -fdebug-compilation-dir=/build -c %s -o %t.o
// RUN: ld.lld -m elf64mmix -e debug_locations %t.o -o %t
// RUN: llvm-dwarfdump --verify %t
// RUN: llvm-dwarfdump --debug-info %t \
// RUN:   | FileCheck %s --check-prefix=INFO

// INFO: DW_AT_name ("debug_locations")
// INFO: DW_AT_location (DW_OP_fbreg
// INFO-NEXT: DW_AT_name ("a0")
// INFO: DW_AT_name ("a15")
// INFO: DW_TAG_formal_parameter
// INFO-NEXT: DW_AT_location (DW_OP_fbreg
// INFO-NEXT: DW_AT_name ("stack_arg")
// INFO: DW_AT_location (DW_OP_fbreg
// INFO-NEXT: DW_AT_name ("signed_byte")
// INFO: DW_AT_type{{.*}}"signed char"
// INFO: DW_AT_location (DW_OP_fbreg
// INFO-NEXT: DW_AT_name ("unsigned_half")
// INFO: DW_AT_type{{.*}}"unsigned short"
// INFO: DW_AT_location (DW_OP_fbreg
// INFO-NEXT: DW_AT_name ("signed_word")
// INFO: DW_AT_type{{.*}}"int"
// INFO: DW_AT_location (DW_OP_fbreg
// INFO-NEXT: DW_AT_name ("signed_octa")
// INFO: DW_AT_type{{.*}}"long"
// INFO: DW_AT_location (DW_OP_fbreg
// INFO-NEXT: DW_AT_name ("pieces")
// INFO: DW_AT_type{{.*}}"pair"

typedef struct {
  long first;
  long second;
} pair;

__attribute__((noinline)) long
debug_locations(long a0, long a1, long a2, long a3, long a4, long a5,
                long a6, long a7, long a8, long a9, long a10, long a11,
                long a12, long a13, long a14, long a15, long stack_arg) {
  signed char signed_byte = (signed char)a0;
  unsigned short unsigned_half = (unsigned short)a1;
  int signed_word = (int)a2;
  long signed_octa = stack_arg;
  pair pieces = {a14, a15};
  return signed_byte + unsigned_half + signed_word + signed_octa +
         pieces.first + pieces.second;
}
