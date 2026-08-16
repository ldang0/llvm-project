// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c17 \
// RUN:   -mrelocation-model static -emit-llvm -disable-llvm-passes \
// RUN:   -o %t.ll %s
// RUN: FileCheck %s --check-prefix=IR < %t.ll
// RUN: FileCheck %s --check-prefix=NO-RAW < %t.ll
// RUN: llc -mtriple=mmix-unknown-elf -verify-machineinstrs \
// RUN:   -stop-after=mmix-isel %t.ll -o - | FileCheck %s --check-prefix=ISEL
// RUN: llc -mtriple=mmix-unknown-elf -verify-machineinstrs -filetype=obj \
// RUN:   %t.ll -o %t.o

// NO-RAW-NOT: va_arg

typedef _Complex float complex_float;
typedef _Complex double complex_double;
typedef _Complex long double complex_long_double;

extern void use_float(complex_float);
extern void use_double(complex_double);
extern void use_long_double(complex_long_double);
extern void variadic_sink(int, ...);

void pass_complex_values(complex_float f, complex_double d,
                         complex_long_double ld) {
  variadic_sink(0, f, d, ld);
}

// IR-LABEL: define dso_local void @pass_complex_values(
// IR-SAME: i64 noundef %f.coerce,
// IR-SAME: ptr noundef byval({ double, double }) align 8 %d,
// IR-SAME: ptr noundef byval({ double, double }) align 8 %ld)
// IR: call void (i32, ...) @variadic_sink(i32 noundef signext 0,
// IR-SAME: i64 noundef %{{[0-9]+}},
// IR-SAME: ptr noundef byval({ double, double }) align 8 %byval-temp,
// IR-SAME: ptr noundef byval({ double, double }) align 8 %byval-temp1)

void consume_complex_values(int named, ...) {
  __builtin_va_list ap;
  __builtin_va_start(ap, named);
  complex_float f = __builtin_va_arg(ap, complex_float);
  complex_double d = __builtin_va_arg(ap, complex_double);
  complex_long_double ld = __builtin_va_arg(ap, complex_long_double);
  __builtin_va_end(ap);
  use_float(f);
  use_double(d);
  use_long_double(ld);
}

// IR-LABEL: define dso_local void @consume_complex_values(
// IR: [[FLOAT_CUR:%[a-z0-9.]+]] = load ptr, ptr %ap, align 8
// IR-NEXT: [[FLOAT_NEXT:%[a-z0-9.]+]] = getelementptr inbounds i8, ptr [[FLOAT_CUR]], i64 8
// IR-NEXT: store ptr [[FLOAT_NEXT]], ptr %ap, align 8
// IR: getelementptr inbounds nuw { float, float }, ptr [[FLOAT_CUR]], i32 0, i32 0
// IR: getelementptr inbounds nuw { float, float }, ptr [[FLOAT_CUR]], i32 0, i32 1
// IR: [[DOUBLE_CUR:%[a-z0-9.]+]] = load ptr, ptr %ap, align 8
// IR-NEXT: [[DOUBLE_NEXT:%[a-z0-9.]+]] = getelementptr inbounds i8, ptr [[DOUBLE_CUR]], i64 8
// IR-NEXT: store ptr [[DOUBLE_NEXT]], ptr %ap, align 8
// IR-NEXT: [[DOUBLE_OBJECT:%[0-9]+]] = load ptr, ptr [[DOUBLE_CUR]], align 8
// IR: getelementptr inbounds nuw { double, double }, ptr [[DOUBLE_OBJECT]], i32 0, i32 0
// IR: getelementptr inbounds nuw { double, double }, ptr [[DOUBLE_OBJECT]], i32 0, i32 1
// IR: [[LONG_DOUBLE_CUR:%[a-z0-9.]+]] = load ptr, ptr %ap, align 8
// IR-NEXT: [[LONG_DOUBLE_NEXT:%[a-z0-9.]+]] = getelementptr inbounds i8, ptr [[LONG_DOUBLE_CUR]], i64 8
// IR-NEXT: store ptr [[LONG_DOUBLE_NEXT]], ptr %ap, align 8
// IR-NEXT: [[LONG_DOUBLE_OBJECT:%[0-9]+]] = load ptr, ptr [[LONG_DOUBLE_CUR]], align 8
// IR: getelementptr inbounds nuw { double, double }, ptr [[LONG_DOUBLE_OBJECT]], i32 0, i32 0
// IR: getelementptr inbounds nuw { double, double }, ptr [[LONG_DOUBLE_OBJECT]], i32 0, i32 1

void consume_float_k15(
    long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7,
    long a8, long a9, long a10, long a11, long a12, long a13, long a14, ...) {
  __builtin_va_list ap;
  __builtin_va_start(ap, a14);
  complex_float value = __builtin_va_arg(ap, complex_float);
  __builtin_va_end(ap);
  use_float(value);
}

// ISEL-LABEL: name: consume_float_k15
// ISEL: [[FLOAT_REG:%[0-9]+]]:{{[^ ]+}} = COPY $r246
// ISEL: STOUI [[FLOAT_REG]], %fixed-stack.0, 0
// ISEL: [[FLOAT_CUR:%[0-9]+]]:{{[^ ]+}} = LDOUI %stack.15.ap, 0
// ISEL: LDTUI [[FLOAT_CUR]], 0
// ISEL: LDTUI [[FLOAT_CUR]], 4

void consume_double_k15(
    long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7,
    long a8, long a9, long a10, long a11, long a12, long a13, long a14, ...) {
  __builtin_va_list ap;
  __builtin_va_start(ap, a14);
  complex_double value = __builtin_va_arg(ap, complex_double);
  __builtin_va_end(ap);
  use_double(value);
}

// ISEL-LABEL: name: consume_double_k15
// ISEL: [[DOUBLE_REG:%[0-9]+]]:{{[^ ]+}} = COPY $r246
// ISEL: STOUI [[DOUBLE_REG]], %fixed-stack.0, 0
// ISEL: [[DOUBLE_CUR:%[0-9]+]]:{{[^ ]+}} = LDOUI %stack.15.ap, 0
// ISEL: [[DOUBLE_OBJECT:%[0-9]+]]:{{[^ ]+}} = LDOUI [[DOUBLE_CUR]], 0
// ISEL: LDOUI [[DOUBLE_OBJECT]], 0
// ISEL: LDOUI [[DOUBLE_OBJECT]], 8

void consume_double_k16(
    long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7,
    long a8, long a9, long a10, long a11, long a12, long a13, long a14,
    long a15, ...) {
  __builtin_va_list ap;
  __builtin_va_start(ap, a15);
  complex_double value = __builtin_va_arg(ap, complex_double);
  __builtin_va_end(ap);
  use_double(value);
}

// ISEL-LABEL: name: consume_double_k16
// ISEL-NOT: STOUI {{.*}}%fixed-stack.0
// ISEL: [[STACK_CURSOR:%[0-9]+]]:{{[^ ]+}} = ADDUI %fixed-stack.0, 0
// ISEL: STOUI {{(killed )?}}[[STACK_CURSOR]], %stack.16.ap, 0
// ISEL: [[STACK_CUR:%[0-9]+]]:{{[^ ]+}} = LDOUI %stack.16.ap, 0
// ISEL: [[STACK_OBJECT:%[0-9]+]]:{{[^ ]+}} = LDOUI [[STACK_CUR]], 0
// ISEL: LDOUI [[STACK_OBJECT]], 0
// ISEL: LDOUI [[STACK_OBJECT]], 8

void call_complex_boundaries(complex_float f, complex_double d) {
  consume_float_k15(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, f);
  consume_double_k15(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, d);
  consume_double_k16(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
                     d);
}

// ISEL-LABEL: name: call_complex_boundaries
// ISEL: $r246 = COPY {{%[0-9]+}}
// ISEL-NEXT: CALL_STATE @consume_float_k15{{.*}}implicit $r246
// ISEL: $r246 = COPY {{%[0-9]+}}
// ISEL-NEXT: CALL_STATE @consume_double_k15{{.*}}implicit $r246
// ISEL: STOUI {{.*}}, {{%[0-9]+}}, 0 :: (store (s64) into stack)
// ISEL: CALL_STATE @consume_double_k16
