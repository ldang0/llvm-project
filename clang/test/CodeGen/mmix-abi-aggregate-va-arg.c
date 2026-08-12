// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=gnu2x \
// RUN:   -mrelocation-model static -emit-llvm -disable-llvm-passes \
// RUN:   -o %t.ll %s
// RUN: FileCheck %s < %t.ll
// RUN: FileCheck %s --check-prefix=NO-RAW < %t.ll
// RUN: llc -mtriple=mmix -verify-machineinstrs -filetype=asm \
// RUN:   %t.ll -o /dev/null
// RUN: llc -mtriple=mmix-unknown-elf -verify-machineinstrs -filetype=obj \
// RUN:   %t.ll -o %t.o

// NO-RAW-NOT: va_arg

struct Empty {};
struct PackedThree {
  unsigned char bytes[3];
} __attribute__((packed));
struct NestedSix {
  unsigned char first;
  struct {
    unsigned short middle;
    unsigned char last;
  } nested;
};
union WordUnion {
  unsigned word;
  unsigned char bytes[4];
};
struct Bits {
  unsigned char low : 3;
  unsigned char high : 5;
};
struct Pair {
  long first;
  long second;
};
struct PackedTwelve {
  unsigned words[3];
} __attribute__((packed));

long consume_aggregates(int named, ...) {
  __builtin_va_list ap;
  __builtin_va_start(ap, named);
  struct Empty empty = __builtin_va_arg(ap, struct Empty);
  struct PackedThree three = __builtin_va_arg(ap, struct PackedThree);
  struct NestedSix nested = __builtin_va_arg(ap, struct NestedSix);
  union WordUnion word = __builtin_va_arg(ap, union WordUnion);
  struct Bits bits = __builtin_va_arg(ap, struct Bits);
  struct Pair pair = __builtin_va_arg(ap, struct Pair);
  struct PackedTwelve twelve = __builtin_va_arg(ap, struct PackedTwelve);
  __builtin_va_end(ap);
  (void)empty;
  return three.bytes[0] + nested.first + word.word + bits.high + pair.second +
         twelve.words[2];
}

// CHECK-LABEL: define dso_local i64 @consume_aggregates(
// CHECK: [[AP:%[a-z0-9.]+]] = alloca ptr, align 8
// CHECK: call void @llvm.va_start.p0(ptr [[AP]])
// CHECK-NEXT: [[THREE_CUR:%[a-z0-9.]+]] = load ptr, ptr [[AP]], align 8
// CHECK: [[THREE_NEXT:%[a-z0-9.]+]] = getelementptr inbounds i8, ptr [[THREE_CUR]], i64 8
// CHECK: store ptr [[THREE_NEXT]], ptr [[AP]], align 8
// CHECK: [[THREE_ADDR:%[a-z0-9.]+]] = getelementptr inbounds i8, ptr [[THREE_CUR]], i64 5
// CHECK: call void @llvm.memcpy.p0.p0.i64(ptr align 1 %three, ptr align 1 [[THREE_ADDR]], i64 3, i1 false)
// CHECK: [[NESTED_CUR:%[a-z0-9.]+]] = load ptr, ptr [[AP]], align 8
// CHECK: [[NESTED_NEXT:%[a-z0-9.]+]] = getelementptr inbounds i8, ptr [[NESTED_CUR]], i64 8
// CHECK: store ptr [[NESTED_NEXT]], ptr [[AP]], align 8
// CHECK: [[NESTED_ADDR:%[a-z0-9.]+]] = getelementptr inbounds i8, ptr [[NESTED_CUR]], i64 2
// CHECK: call void @llvm.memcpy.p0.p0.i64(ptr align 2 %nested, ptr align 2 [[NESTED_ADDR]], i64 6, i1 false)
// CHECK: [[WORD_CUR:%[a-z0-9.]+]] = load ptr, ptr [[AP]], align 8
// CHECK: [[WORD_NEXT:%[a-z0-9.]+]] = getelementptr inbounds i8, ptr [[WORD_CUR]], i64 8
// CHECK: store ptr [[WORD_NEXT]], ptr [[AP]], align 8
// CHECK: [[WORD_ADDR:%[a-z0-9.]+]] = getelementptr inbounds i8, ptr [[WORD_CUR]], i64 4
// CHECK: call void @llvm.memcpy.p0.p0.i64(ptr align 4 %word, ptr align 4 [[WORD_ADDR]], i64 4, i1 false)
// CHECK: [[BITS_CUR:%[a-z0-9.]+]] = load ptr, ptr [[AP]], align 8
// CHECK: [[BITS_NEXT:%[a-z0-9.]+]] = getelementptr inbounds i8, ptr [[BITS_CUR]], i64 8
// CHECK: store ptr [[BITS_NEXT]], ptr [[AP]], align 8
// CHECK: [[BITS_ADDR:%[a-z0-9.]+]] = getelementptr inbounds i8, ptr [[BITS_CUR]], i64 7
// CHECK: call void @llvm.memcpy.p0.p0.i64(ptr align 1 %bits, ptr align 1 [[BITS_ADDR]], i64 1, i1 false)
// CHECK: [[PAIR_CUR:%[a-z0-9.]+]] = load ptr, ptr [[AP]], align 8
// CHECK: [[PAIR_NEXT:%[a-z0-9.]+]] = getelementptr inbounds i8, ptr [[PAIR_CUR]], i64 8
// CHECK: store ptr [[PAIR_NEXT]], ptr [[AP]], align 8
// CHECK: [[PAIR_PTR:%[a-z0-9.]+]] = load ptr, ptr [[PAIR_CUR]], align 8
// CHECK: call void @llvm.memcpy.p0.p0.i64(ptr align 8 %pair, ptr align 8 [[PAIR_PTR]], i64 16, i1 false)
// CHECK: [[TWELVE_CUR:%[a-z0-9.]+]] = load ptr, ptr [[AP]], align 8
// CHECK: [[TWELVE_NEXT:%[a-z0-9.]+]] = getelementptr inbounds i8, ptr [[TWELVE_CUR]], i64 8
// CHECK: store ptr [[TWELVE_NEXT]], ptr [[AP]], align 8
// CHECK: [[TWELVE_PTR:%[a-z0-9.]+]] = load ptr, ptr [[TWELVE_CUR]], align 8
// CHECK: call void @llvm.memcpy.p0.p0.i64(ptr align 1 %twelve, ptr align 1 [[TWELVE_PTR]], i64 12, i1 false)
// CHECK: call void @llvm.va_end.p0(ptr [[AP]])
