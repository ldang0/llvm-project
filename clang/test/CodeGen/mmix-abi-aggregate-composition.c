// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -mrelocation-model static \
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

// CHECK: %struct.Large = type { [3 x i64] }
// CHECK: %struct.Direct = type { i32 }
// CHECK: %struct.Empty = type {}

typedef struct Large (*large_transform)(long, struct Empty, struct Direct,
                                        struct Large);

struct Direct external_direct(struct Empty empty, struct Direct seed,
                              struct Large copy);
struct Large external_large(long depth, struct Empty empty,
                            struct Direct seed, struct Large copy);

struct Direct forward_direct(struct Empty empty, struct Direct seed,
                             struct Large copy) {
  return external_direct(empty, seed, copy);
}

// CHECK-LABEL: define dso_local i32 @forward_direct(
// CHECK-SAME: i32 noext %seed.coerce,
// CHECK-SAME: ptr noundef byval(%struct.Large) align 8 %copy)
// CHECK: [[DIRECT:%[a-z0-9.]+]] = call i32 @external_direct(
// CHECK-SAME: i32 noext %{{[0-9]+}},
// CHECK-SAME: ptr noundef byval(%struct.Large) align 8 %copy)
// CHECK: store i32 [[DIRECT]], ptr %coerce.dive2, align 4
// CHECK: [[DIRECT_VALUE:%[0-9]+]] = load i32, ptr %coerce.dive3, align 4
// CHECK: ret i32 [[DIRECT_VALUE]]
// CHECK: declare dso_local i32 @external_direct(
// CHECK-SAME: i32 noext, ptr noundef byval(%struct.Large) align 8)

struct Large recursive_large(long depth, struct Empty empty,
                             struct Direct seed, struct Large copy) {
  if (depth == 0)
    return copy;
  return recursive_large(depth - 1, empty, seed, copy);
}

// CHECK-LABEL: define dso_local void @recursive_large(
// CHECK-SAME: ptr dead_on_unwind noalias writable sret(%struct.Large) align 8 %agg.result,
// CHECK-SAME: i64 noundef %depth, i32 noext %seed.coerce,
// CHECK-SAME: ptr noundef byval(%struct.Large) align 8 %copy)
// CHECK: call void @recursive_large(
// CHECK-SAME: ptr dead_on_unwind writable sret(%struct.Large) align 8 %agg.result,
// CHECK-SAME: i64 noundef %sub, i32 noext %{{[0-9]+}},
// CHECK-SAME: ptr noundef byval(%struct.Large) align 8 %copy)

struct Large call_external(long depth, struct Empty empty, struct Direct seed,
                           struct Large copy) {
  return external_large(depth, empty, seed, copy);
}

// CHECK-LABEL: define dso_local void @call_external(
// CHECK-SAME: ptr dead_on_unwind noalias writable sret(%struct.Large) align 8 %agg.result,
// CHECK-SAME: i64 noundef %depth, i32 noext %seed.coerce,
// CHECK-SAME: ptr noundef byval(%struct.Large) align 8 %copy)
// CHECK: call void @external_large(
// CHECK-SAME: ptr dead_on_unwind writable sret(%struct.Large) align 8 %agg.result,
// CHECK-SAME: i64 noundef %{{[0-9]+}}, i32 noext %{{[0-9]+}},
// CHECK-SAME: ptr noundef byval(%struct.Large) align 8 %copy)
// CHECK: declare dso_local void @external_large(
// CHECK-SAME: ptr dead_on_unwind writable sret(%struct.Large) align 8,
// CHECK-SAME: i64 noundef, i32 noext,
// CHECK-SAME: ptr noundef byval(%struct.Large) align 8)

struct Large call_indirect(large_transform transform, long depth,
                           struct Empty empty, struct Direct seed,
                           struct Large copy) {
  return transform(depth, empty, seed, copy);
}

// CHECK-LABEL: define dso_local void @call_indirect(
// CHECK-SAME: ptr dead_on_unwind noalias writable sret(%struct.Large) align 8 %agg.result,
// CHECK-SAME: ptr noundef %transform, i64 noundef %depth,
// CHECK-SAME: i32 noext %seed.coerce,
// CHECK-SAME: ptr noundef byval(%struct.Large) align 8 %copy)
// CHECK: call void %{{[0-9]+}}(
// CHECK-SAME: ptr dead_on_unwind writable sret(%struct.Large) align 8 %agg.result,
// CHECK-SAME: i64 noundef %{{[0-9]+}}, i32 noext %{{[0-9]+}},
// CHECK-SAME: ptr noundef byval(%struct.Large) align 8 %copy)

struct Large mixed_boundary(
    long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7,
    long a8, long a9, long a10, long a11, long a12, long a13, long a14,
    struct Empty empty, struct Direct direct, struct Large copy, long tail) {
  if (tail)
    return copy;
  return external_large(a0, empty, direct, copy);
}

// The sret pointer and empty value consume no ordinary argument slot. The
// direct value is slot 15, the byval pointer is slot 16, and tail is slot 17.
// CHECK-LABEL: define dso_local void @mixed_boundary(
// CHECK-SAME: ptr dead_on_unwind noalias writable sret(%struct.Large) align 8 %agg.result,
// CHECK-SAME: i64 noundef %a0, i64 noundef %a1, i64 noundef %a2,
// CHECK-SAME: i64 noundef %a3, i64 noundef %a4, i64 noundef %a5,
// CHECK-SAME: i64 noundef %a6, i64 noundef %a7, i64 noundef %a8,
// CHECK-SAME: i64 noundef %a9, i64 noundef %a10, i64 noundef %a11,
// CHECK-SAME: i64 noundef %a12, i64 noundef %a13, i64 noundef %a14,
// CHECK-SAME: i32 noext %direct.coerce,
// CHECK-SAME: ptr noundef byval(%struct.Large) align 8 %copy,
// CHECK-SAME: i64 noundef %tail)
