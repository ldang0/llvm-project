// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=gnu2x \
// RUN:   -mrelocation-model static -emit-llvm -disable-llvm-passes \
// RUN:   -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=gnu2x \
// RUN:   -mrelocation-model static -emit-llvm -disable-llvm-passes \
// RUN:   -o - %s | llc -mtriple=mmix -stop-after=prolog-epilog -o - \
// RUN:   | FileCheck %s --check-prefix=FRAME
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=gnu2x \
// RUN:   -mrelocation-model static -emit-llvm -disable-llvm-passes \
// RUN:   -DOVERALIGNED -o - %s \
// RUN:   | not llc -mtriple=mmix -o /dev/null 2>&1 \
// RUN:   | FileCheck %s --check-prefix=OVERALIGNED
// RUN: not %clang_cc1 -triple mmix-unknown-unknown -std=gnu2x \
// RUN:   -mrelocation-model static -emit-llvm -disable-llvm-passes \
// RUN:   -DVARIABLE -o /dev/null %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=VARIABLE

struct Packed {
  char first;
  short second;
} __attribute__((packed));

_Static_assert(_Alignof(char) == 1, "char type alignment");
_Static_assert(_Alignof(short) == 2, "short type alignment");
_Static_assert(_Alignof(struct Packed) == 1, "packed type alignment");
_Static_assert(sizeof(char[3]) == 3, "narrow array size");
_Static_assert(sizeof(struct Packed) == 3, "packed object size");
_Static_assert(__builtin_offsetof(struct Packed, second) == 1,
               "packed member offset");

void fixed_automatic_objects(void) {
  char byte;
  short half;
  char bytes[3];
  struct Packed packed;
  int word;
  long octa;
  char aligned_eight __attribute__((aligned(8)));
  byte = bytes[0] = packed.first = aligned_eight = 1;
  half = packed.second = 2;
  word = 3;
  octa = 4;
}

// CHECK-LABEL: define dso_local void @fixed_automatic_objects()
// CHECK-DAG: %byte = alloca i8, align 1
// CHECK-DAG: %half = alloca i16, align 2
// CHECK-DAG: %bytes = alloca [3 x i8], align 1
// CHECK-DAG: %packed = alloca %struct.Packed, align 1
// CHECK-DAG: %word = alloca i32, align 4
// CHECK-DAG: %octa = alloca i64, align 8
// CHECK-DAG: %aligned_eight = alloca i8, align 8

// FRAME-LABEL: name: fixed_automatic_objects
// FRAME: stack:
// FRAME: name: byte,{{.*}}size: 1, alignment: 4
// FRAME: name: half,{{.*}}size: 2, alignment: 4
// FRAME: name: bytes,{{.*}}size: 3, alignment: 4
// FRAME: name: packed,{{.*}}size: 3, alignment: 4
// FRAME: name: word,{{.*}}size: 4, alignment: 4
// FRAME: name: octa,{{.*}}size: 8, alignment: 8
// FRAME: name: aligned_eight,{{.*}}size: 1,
// FRAME-NEXT: alignment: 8,

#ifdef OVERALIGNED
void unsupported_alignment(void) {
  char value __attribute__((aligned(16)));
  value = 0;
}

// OVERALIGNED: LLVM ERROR: MMIX does not support stack realignment in function 'unsupported_alignment'
#endif

#ifdef VARIABLE
void unsupported_variable_size(int count) {
  char values[count];
  values[0] = 0;
}

// VARIABLE: error: variable length arrays are not supported for the current target
#endif
