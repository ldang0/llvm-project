// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=gnu2x \
// RUN:   -mrelocation-model static -emit-llvm -disable-llvm-passes \
// RUN:   -verify -o %t.ll %s
// RUN: FileCheck %s < %t.ll
// RUN: FileCheck %s --check-prefix=NO-RAW < %t.ll
// RUN: llc -mtriple=mmix -verify-machineinstrs -filetype=asm \
// RUN:   %t.ll -o /dev/null
// RUN: llc -mtriple=mmix-unknown-elf -verify-machineinstrs -filetype=obj \
// RUN:   %t.ll -o %t.o

// NO-RAW-NOT: va_arg

long consume_scalars(int named, ...) {
  __builtin_va_list ap;
  __builtin_va_list copy;
  __builtin_va_start(ap, named);
  __builtin_va_copy(copy, ap);
  int signed_word = __builtin_va_arg(ap, int);
  unsigned unsigned_word = __builtin_va_arg(ap, unsigned);
  long wide = __builtin_va_arg(ap, long);
  void *pointer = __builtin_va_arg(ap, void *);
  double real = __builtin_va_arg(ap, double);
  int copied_word = __builtin_va_arg(copy, int);
  __builtin_va_end(copy);
  __builtin_va_end(ap);
  return signed_word + unsigned_word + wide + (pointer != 0) + (long)real +
         copied_word;
}

// CHECK-LABEL: define dso_local i64 @consume_scalars(
// CHECK: [[AP:%[a-z0-9.]+]] = alloca ptr, align 8
// CHECK: [[COPY:%[a-z0-9.]+]] = alloca ptr, align 8
// CHECK: call void @llvm.va_start.p0(ptr [[AP]])
// CHECK: call void @llvm.va_copy.p0(ptr [[COPY]], ptr [[AP]])
// CHECK: [[SIGNED_CUR:%[a-z0-9.]+]] = load ptr, ptr [[AP]], align 8
// CHECK: [[SIGNED_NEXT:%[a-z0-9.]+]] = getelementptr inbounds i8, ptr [[SIGNED_CUR]], i64 8
// CHECK: store ptr [[SIGNED_NEXT]], ptr [[AP]], align 8
// CHECK: [[SIGNED_ADDR:%[a-z0-9.]+]] = getelementptr inbounds i8, ptr [[SIGNED_CUR]], i64 4
// CHECK: load i32, ptr [[SIGNED_ADDR]], align 4
// CHECK: [[UNSIGNED_CUR:%[a-z0-9.]+]] = load ptr, ptr [[AP]], align 8
// CHECK: [[UNSIGNED_NEXT:%[a-z0-9.]+]] = getelementptr inbounds i8, ptr [[UNSIGNED_CUR]], i64 8
// CHECK: store ptr [[UNSIGNED_NEXT]], ptr [[AP]], align 8
// CHECK: [[UNSIGNED_ADDR:%[a-z0-9.]+]] = getelementptr inbounds i8, ptr [[UNSIGNED_CUR]], i64 4
// CHECK: load i32, ptr [[UNSIGNED_ADDR]], align 4
// CHECK: [[WIDE_CUR:%[a-z0-9.]+]] = load ptr, ptr [[AP]], align 8
// CHECK: [[WIDE_NEXT:%[a-z0-9.]+]] = getelementptr inbounds i8, ptr [[WIDE_CUR]], i64 8
// CHECK: store ptr [[WIDE_NEXT]], ptr [[AP]], align 8
// CHECK: load i64, ptr [[WIDE_CUR]], align 8
// CHECK: [[POINTER_CUR:%[a-z0-9.]+]] = load ptr, ptr [[AP]], align 8
// CHECK: [[POINTER_NEXT:%[a-z0-9.]+]] = getelementptr inbounds i8, ptr [[POINTER_CUR]], i64 8
// CHECK: store ptr [[POINTER_NEXT]], ptr [[AP]], align 8
// CHECK: load ptr, ptr [[POINTER_CUR]], align 8
// CHECK: [[DOUBLE_CUR:%[a-z0-9.]+]] = load ptr, ptr [[AP]], align 8
// CHECK: [[DOUBLE_NEXT:%[a-z0-9.]+]] = getelementptr inbounds i8, ptr [[DOUBLE_CUR]], i64 8
// CHECK: store ptr [[DOUBLE_NEXT]], ptr [[AP]], align 8
// CHECK: load double, ptr [[DOUBLE_CUR]], align 8
// CHECK: [[COPY_CUR:%[a-z0-9.]+]] = load ptr, ptr [[COPY]], align 8
// CHECK: [[COPY_NEXT:%[a-z0-9.]+]] = getelementptr inbounds i8, ptr [[COPY_CUR]], i64 8
// CHECK: store ptr [[COPY_NEXT]], ptr [[COPY]], align 8
// CHECK: [[COPY_ADDR:%[a-z0-9.]+]] = getelementptr inbounds i8, ptr [[COPY_CUR]], i64 4
// CHECK: load i32, ptr [[COPY_ADDR]], align 4
// CHECK: call void @llvm.va_end.p0(ptr [[COPY]])
// CHECK: call void @llvm.va_end.p0(ptr [[AP]])

long no_named(...) {
  __builtin_va_list ap;
  __builtin_c23_va_start(ap);
  long value = __builtin_va_arg(ap, long);
  __builtin_va_end(ap);
  return value;
}

// CHECK-LABEL: define dso_local i64 @no_named(...)
// CHECK: call void @llvm.va_start.p0(
// CHECK: getelementptr inbounds i8, ptr %{{[a-z0-9.]+}}, i64 8
// CHECK: load i64,
// CHECK: call void @llvm.va_end.p0(

long at_register_boundary(
    long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7,
    long a8, long a9, long a10, long a11, long a12, long a13, long a14,
    long a15, ...) {
  __builtin_va_list ap;
  __builtin_va_start(ap, a15);
  long value = __builtin_va_arg(ap, long);
  __builtin_va_end(ap);
  return value;
}

long above_register_boundary(
    long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7,
    long a8, long a9, long a10, long a11, long a12, long a13, long a14,
    long a15, long a16, ...) {
  __builtin_va_list ap;
  __builtin_va_start(ap, a16);
  long value = __builtin_va_arg(ap, long);
  __builtin_va_end(ap);
  return value;
}

// CHECK-LABEL: define dso_local i64 @at_register_boundary(
// CHECK: call void @llvm.va_start.p0(
// CHECK: load i64,
// CHECK-LABEL: define dso_local i64 @above_register_boundary(
// CHECK: call void @llvm.va_start.p0(
// CHECK: load i64,

void mismatched_promotions(int named, ...) {
  __builtin_va_list ap;
  __builtin_va_start(ap, named);
  (void)__builtin_va_arg(ap, short); // expected-warning {{second argument to 'va_arg' is of promotable type 'short'}}
  (void)__builtin_va_arg(ap, float); // expected-warning {{second argument to 'va_arg' is of promotable type 'float'}}
  __builtin_va_end(ap);
}

// CHECK-LABEL: define dso_local void @mismatched_promotions(
// CHECK: getelementptr inbounds i8, ptr %{{[a-z0-9.]+}}, i64 6
// CHECK: load i16,
// CHECK: getelementptr inbounds i8, ptr %{{[a-z0-9.]+}}, i64 4
// CHECK: load float,
