// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=gnu17 \
// RUN:   -Wno-deprecated-non-prototype -mrelocation-model static \
// RUN:   -emit-llvm -disable-llvm-passes -o %t.ll %s
// RUN: FileCheck %s < %t.ll
// RUN: llc -mtriple=mmix -verify-machineinstrs -filetype=asm \
// RUN:   %t.ll -o /dev/null
// RUN: llc -mtriple=mmix-unknown-elf -verify-machineinstrs -filetype=obj \
// RUN:   %t.ll -o %t.o

struct Empty {};
struct Direct {
  int word;
};
struct Large {
  long words[3];
};

void old_style();

void call_old_style(signed char signed_byte, unsigned int unsigned_word,
                    float single, void *pointer, long wide,
                    struct Direct direct, struct Large copy,
                    struct Empty empty) {
  old_style(signed_byte, unsigned_word, single, pointer, wide, direct, copy,
            empty);
}

// CHECK-LABEL: define dso_local void @call_old_style(
// CHECK: call void @old_style(
// CHECK-SAME: i32 noundef signext %{{[a-z0-9.]+}},
// CHECK-SAME: i32 noundef zeroext %{{[a-z0-9.]+}},
// CHECK-SAME: double noundef %{{[a-z0-9.]+}},
// CHECK-SAME: ptr noundef %{{[a-z0-9.]+}}, i64 noundef %{{[a-z0-9.]+}},
// CHECK-SAME: i32 noext %{{[a-z0-9.]+}},
// CHECK-SAME: ptr noundef byval(%struct.Large) align 8 %copy)
