// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=gnu2x \
// RUN:   -mrelocation-model static -emit-llvm -disable-llvm-passes -o %t.ll %s
// RUN: FileCheck %s < %t.ll
// RUN: llc -mtriple=mmix -verify-machineinstrs -filetype=asm \
// RUN:   %t.ll -o /dev/null
// RUN: llc -mtriple=mmix-unknown-elf -verify-machineinstrs -filetype=obj \
// RUN:   %t.ll -o %t.o

struct Empty {};
struct Tiny {
  char byte;
};
struct Direct {
  int word;
};
struct Large {
  long words[3];
};

void zero_fixed(...);
long one_fixed(long fixed, ...);
struct Large indirect_result(long fixed, ...);

long defined_variadic(long fixed, ...) { return fixed; }

// CHECK-LABEL: define dso_local i64 @defined_variadic(i64 noundef %fixed, ...)

void classify_narrow_direct(struct Tiny tiny) { zero_fixed(tiny); }

// CHECK-LABEL: define dso_local void @classify_narrow_direct(i8 noext %tiny.coerce)
// CHECK: call void (...) @zero_fixed(i8 noext %{{[a-z0-9.]+}})

void classify_unnamed(signed char signed_byte, unsigned int unsigned_word,
                      float single, void *pointer, long wide,
                      struct Direct direct, struct Large copy,
                      struct Empty empty) {
  zero_fixed(signed_byte, unsigned_word, single, pointer, wide, direct, copy,
             empty);
  (void)one_fixed(0, signed_byte, unsigned_word, single, pointer, wide, direct,
                  copy, empty);
}

// CHECK-LABEL: define dso_local void @classify_unnamed(
// CHECK: call void (...) @zero_fixed(
// CHECK-SAME: i32 noundef signext %{{[a-z0-9.]+}},
// CHECK-SAME: i32 noundef zeroext %{{[a-z0-9.]+}},
// CHECK-SAME: double noundef %{{[a-z0-9.]+}},
// CHECK-SAME: ptr noundef %{{[a-z0-9.]+}}, i64 noundef %{{[a-z0-9.]+}},
// CHECK-SAME: i32 noext %{{[a-z0-9.]+}},
// CHECK-SAME: ptr noundef byval(%struct.Large) align 8 %copy)
// CHECK: call i64 (i64, ...) @one_fixed(i64 noundef 0,
// CHECK-SAME: i32 noundef signext %{{[a-z0-9.]+}},
// CHECK-SAME: i32 noundef zeroext %{{[a-z0-9.]+}},
// CHECK-SAME: double noundef %{{[a-z0-9.]+}},
// CHECK-SAME: ptr noundef %{{[a-z0-9.]+}}, i64 noundef %{{[a-z0-9.]+}},
// CHECK-SAME: i32 noext %{{[a-z0-9.]+}},
// CHECK-SAME: ptr noundef byval(%struct.Large) align 8 %copy)

struct Large call_indirect_result(int value) {
  return indirect_result(0, value);
}

// CHECK-LABEL: define dso_local void @call_indirect_result(
// CHECK-SAME: ptr dead_on_unwind noalias writable sret(%struct.Large) align 8 %agg.result,
// CHECK-SAME: i32 noundef signext %value)
// CHECK: call void (ptr, i64, ...) @indirect_result(
// CHECK-SAME: ptr dead_on_unwind writable sret(%struct.Large) align 8 %agg.result,
// CHECK-SAME: i64 noundef 0, i32 noundef signext %{{[a-z0-9.]+}})

long k15(long a0, long a1, long a2, long a3, long a4, long a5, long a6,
         long a7, long a8, long a9, long a10, long a11, long a12, long a13,
         long a14, ...);
long k16(long a0, long a1, long a2, long a3, long a4, long a5, long a6,
         long a7, long a8, long a9, long a10, long a11, long a12, long a13,
         long a14, long a15, ...);
long k17(long a0, long a1, long a2, long a3, long a4, long a5, long a6,
         long a7, long a8, long a9, long a10, long a11, long a12, long a13,
         long a14, long a15, long a16, ...);

long call_boundaries(int unnamed) {
  long result = k15(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
                    unnamed);
  result += k16(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
                unnamed);
  return result + k17(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
                      16, unnamed);
}

// CHECK-LABEL: define dso_local i64 @call_boundaries(i32 noundef signext %unnamed)
// CHECK: call i64 (i64, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64, ...) @k15({{.*}}i32 noundef signext %{{[a-z0-9.]+}})
// CHECK: call i64 (i64, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64, ...) @k16({{.*}}i32 noundef signext %{{[a-z0-9.]+}})
// CHECK: call i64 (i64, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64, ...) @k17({{.*}}i32 noundef signext %{{[a-z0-9.]+}})
