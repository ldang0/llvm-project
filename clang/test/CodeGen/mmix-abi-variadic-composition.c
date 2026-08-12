// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=gnu2x \
// RUN:   -mrelocation-model static -emit-llvm -disable-llvm-passes \
// RUN:   -o %t.ll %s
// RUN: FileCheck %s --check-prefix=IR < %t.ll
// RUN: FileCheck %s --check-prefix=NO-RAW < %t.ll
// RUN: llc -mtriple=mmix-unknown-elf -verify-machineinstrs \
// RUN:   -stop-after=mmix-isel %t.ll -o - | FileCheck %s --check-prefix=ISEL
// RUN: llc -mtriple=mmix-unknown-elf -verify-machineinstrs -filetype=asm \
// RUN:   %t.ll -o - | FileCheck %s --check-prefix=ASM
// RUN: llc -mtriple=mmix-unknown-elf -verify-machineinstrs -filetype=obj \
// RUN:   %t.ll -o %t.o

// NO-RAW-NOT: va_arg

struct Direct {
  int word;
};
struct Large {
  long first;
  long second;
  long third;
};

extern long observe(long);

long consume_k0(...) {
  __builtin_va_list ap;
  __builtin_c23_va_start(ap);
  long value = __builtin_va_arg(ap, long);
  __builtin_va_end(ap);
  return value;
}

// IR-LABEL: define dso_local i64 @consume_k0(...)
// IR: call void @llvm.va_start.p0(
// IR: getelementptr inbounds i8, ptr %{{[a-z0-9.]+}}, i64 8
// IR: load i64,
// ISEL-LABEL: name: consume_k0
// ISEL: [[K0_ARG:%[0-9]+]]:{{[^ ]+}} = COPY $r231
// ISEL: STOUI [[K0_ARG]], %fixed-stack.0, 0
// ISEL: [[K0_CURSOR:%[0-9]+]]:{{[^ ]+}} = ADDUI %fixed-stack.0, 0

long consume_k1(long named, ...) {
  __builtin_va_list ap;
  __builtin_va_list copy;
  __builtin_va_start(ap, named);
  __builtin_va_copy(copy, ap);
  long first = __builtin_va_arg(ap, long);
  long copied = __builtin_va_arg(copy, long);
  long second = __builtin_va_arg(ap, long);
  __builtin_va_end(copy);
  __builtin_va_end(ap);
  return first + copied + second;
}

// IR-LABEL: define dso_local i64 @consume_k1(i64 noundef %named, ...)
// IR: call void @llvm.va_start.p0(
// IR: call void @llvm.va_copy.p0(
// IR: [[FIRST_NEXT:%[a-z0-9.]+]] = getelementptr inbounds i8, ptr [[FIRST_CUR:%[a-z0-9.]+]], i64 8
// IR: [[COPY_NEXT:%[a-z0-9.]+]] = getelementptr inbounds i8, ptr [[COPY_CUR:%[a-z0-9.]+]], i64 8
// IR: [[SECOND_NEXT:%[a-z0-9.]+]] = getelementptr inbounds i8, ptr [[SECOND_CUR:%[a-z0-9.]+]], i64 8
// ISEL-LABEL: name: consume_k1
// ISEL: [[K1_ARG:%[0-9]+]]:{{[^ ]+}} = COPY $r232
// ISEL: STOUI [[K1_ARG]], %fixed-stack.0, 0

long consume_k15(
    long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7,
    long a8, long a9, long a10, long a11, long a12, long a13, long a14, ...) {
  __builtin_va_list ap;
  __builtin_va_start(ap, a14);
  long nested = observe(a0);
  struct Direct direct = __builtin_va_arg(ap, struct Direct);
  struct Large large = __builtin_va_arg(ap, struct Large);
  __builtin_va_end(ap);
  return nested + direct.word + large.second;
}

// IR-LABEL: define dso_local i64 @consume_k15(
// IR-SAME: i64 noundef %a14, ...)
// IR: call void @llvm.va_start.p0(
// IR: call i64 @observe(i64 noundef %{{[a-z0-9.]+}})
// IR: getelementptr inbounds i8, ptr %{{[a-z0-9.]+}}, i64 4
// IR: load ptr, ptr %{{[a-z0-9.]+}}, align 8
// IR: call void @llvm.memcpy.p0.p0.i64({{.*}}i64 24, i1 false)
// ISEL-LABEL: name: consume_k15
// ISEL: [[K15_ARG:%[0-9]+]]:{{[^ ]+}} = COPY $r246
// ISEL: STOUI [[K15_ARG]], %fixed-stack.0, 0
// ISEL: DIRECT_CALL_STATE @observe
// ISEL: LDTUI {{%[0-9]+}}, 4
// ISEL: [[LARGE_PTR:%[0-9]+]]:{{[^ ]+}} = LDOUI {{%[0-9]+}}, 0
// ISEL: LDOUI {{(killed )?}}[[LARGE_PTR]], 8

long consume_k16(
    long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7,
    long a8, long a9, long a10, long a11, long a12, long a13, long a14,
    long a15, ...) {
  __builtin_va_list ap;
  __builtin_va_start(ap, a15);
  long value = __builtin_va_arg(ap, long);
  __builtin_va_end(ap);
  return value;
}

// IR-LABEL: define dso_local i64 @consume_k16(
// IR: call void @llvm.va_start.p0(
// ISEL-LABEL: name: consume_k16
// ISEL-NOT: STOUI {{.*}}%fixed-stack.0
// ISEL: [[K16_CURSOR:%[0-9]+]]:{{[^ ]+}} = ADDUI %fixed-stack.0, 0
// ISEL: STOUI {{(killed )?}}[[K16_CURSOR]], %stack.16.ap, 0
// ISEL: [[K16_CURRENT:%[0-9]+]]:{{[^ ]+}} = LDOUI %stack.16.ap, 0
// ISEL: LDOUI [[K16_CURRENT]], 0

long consume_k17(
    long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7,
    long a8, long a9, long a10, long a11, long a12, long a13, long a14,
    long a15, long a16, ...) {
  __builtin_va_list ap;
  __builtin_va_start(ap, a16);
  long value = __builtin_va_arg(ap, long);
  __builtin_va_end(ap);
  return value;
}

// IR-LABEL: define dso_local i64 @consume_k17(
// IR: call void @llvm.va_start.p0(
// ISEL-LABEL: name: consume_k17
// ISEL-NOT: STOUI {{.*}}%fixed-stack.1
// ISEL: [[K17_CURSOR:%[0-9]+]]:{{[^ ]+}} = ADDUI %fixed-stack.1, 0
// ISEL: STOUI {{(killed )?}}[[K17_CURSOR]], %stack.17.ap, 0
// ISEL: [[K17_CURRENT:%[0-9]+]]:{{[^ ]+}} = LDOUI %stack.17.ap, 0
// ISEL: LDOUI [[K17_CURRENT]], 0

struct Large return_large(long named, ...) {
  __builtin_va_list ap;
  __builtin_va_start(ap, named);
  struct Large value = __builtin_va_arg(ap, struct Large);
  __builtin_va_end(ap);
  return value;
}

// IR-LABEL: define dso_local void @return_large(
// IR-SAME: ptr dead_on_unwind noalias writable sret(%struct.Large) align 8 %agg.result,
// IR-SAME: i64 noundef %named, ...)
// IR: load ptr, ptr %{{[a-z0-9.]+}}, align 8
// IR: call void @llvm.memcpy.p0.p0.i64({{.*}}i64 24, i1 false)
// ISEL-LABEL: name: return_large
// ISEL: [[SRET:%[0-9]+]]:{{[^ ]+}} = COPY $r251
// ISEL: [[SRET_VARARG:%[0-9]+]]:{{[^ ]+}} = COPY $r232
// ISEL: STOUI [[SRET_VARARG]], %fixed-stack.0, 0
// ISEL: [[SRET_OBJECT:%[0-9]+]]:{{[^ ]+}} = LDOUI {{%[0-9]+}}, 0
// ISEL: LDOUI [[SRET_OBJECT]], 16
// ISEL: LDOUI [[SRET_OBJECT]], 8
// ISEL: LDOUI [[SRET_OBJECT]], 0

long compose_calls(struct Direct direct, struct Large large) {
  long result = consume_k0(10L);
  result += consume_k1(0, 11L, 12L);
  result += consume_k15(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
                        14, direct, large);
  result += consume_k16(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
                        14, 15, 16L);
  result += consume_k17(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
                        14, 15, 16, 17L);
  struct Large returned = return_large(0, large);
  return result + returned.first;
}

// IR-LABEL: define dso_local i64 @compose_calls(
// IR: call i64 (...) @consume_k0(i64 noundef 10)
// IR: call i64 (i64, ...) @consume_k1(i64 noundef 0, i64 noundef 11, i64 noundef 12)
// IR: call i64 ({{.*}}) @consume_k15({{.*}}i32 noext %{{[a-z0-9.]+}}, ptr noundef byval(%struct.Large) align 8 %large)
// IR: call i64 ({{.*}}) @consume_k16({{.*}}i64 noundef 16)
// IR: call i64 ({{.*}}) @consume_k17({{.*}}i64 noundef 17)
// IR: call void (ptr, i64, ...) @return_large(ptr dead_on_unwind writable sret(%struct.Large) align 8 %returned, i64 noundef 0, ptr noundef byval(%struct.Large) align 8 %large)
// ISEL-LABEL: name: compose_calls
// ISEL: CALL_STATE @consume_k0{{.*}}implicit $r231
// ISEL: CALL_STATE @consume_k1{{.*}}implicit $r231, implicit $r232, implicit $r233
// ISEL: CALL_STATE @consume_k15{{.*}}implicit $r246
// ISEL: CALL_STATE @consume_k16
// ISEL: CALL_STATE @consume_k17
// ISEL: CALL_STATE @return_large{{.*}}implicit $r251, implicit $r231, implicit $r232

// ASM-LABEL: consume_k0:
// ASM: STOU r231,
// ASM-LABEL: consume_k15:
// ASM: STOU r246,
// ASM: PUSHGO
// ASM-LABEL: compose_calls:
// ASM: PUSHJB {{r[0-9]+}}, consume_k0
