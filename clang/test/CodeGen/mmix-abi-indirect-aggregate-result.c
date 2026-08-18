// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -mrelocation-model static \
// RUN:   -emit-llvm -disable-llvm-passes -o %t.ll %s
// RUN: FileCheck %s < %t.ll
// RUN: llc -mtriple=mmix -verify-machineinstrs -stop-after=mmix-isel \
// RUN:   %t.ll -o - | FileCheck %s --check-prefix=ISEL
// RUN: llc -mtriple=mmix -verify-machineinstrs -filetype=obj \
// RUN:   %t.ll -o /dev/null

struct PackedThree {
  char bytes[3];
} __attribute__((packed));
struct PaddedEight {
  char head;
  int tail;
};
struct Triple {
  long words[3];
};
struct PackedWord {
  int value;
} __attribute__((packed));
typedef struct {
  long quot;
  long rem;
} imaxdiv_t;

struct PackedThree make_packed(char a, char b, char c) {
  return (struct PackedThree){{a, b, c}};
}

// CHECK-LABEL: define dso_local void @make_packed(
// CHECK-SAME: ptr dead_on_unwind noalias writable sret(%struct.PackedThree) align 1 %agg.result,
// CHECK-SAME: i8 noundef signext %a, i8 noundef signext %b,
// CHECK-SAME: i8 noundef signext %c)
// CHECK-NOT: byval
// CHECK-NOT: inreg
// CHECK-NOT: returned
// CHECK: store i8 %{{[0-9]+}}, ptr %bytes, align 1

struct PaddedEight make_padded(char head, int tail) {
  return (struct PaddedEight){head, tail};
}

// CHECK-LABEL: define dso_local void @make_padded(
// CHECK-SAME: ptr dead_on_unwind noalias writable sret(%struct.PaddedEight) align 4 %agg.result,

struct Triple make_triple(long value) {
  return (struct Triple){{value, value + 1, value + 2}};
}

// CHECK-LABEL: define dso_local void @make_triple(
// CHECK-SAME: ptr dead_on_unwind noalias writable sret(%struct.Triple) align 8 %agg.result,
// CHECK-SAME: i64 noundef %value)

struct PackedWord make_packed_word(int value) {
  return (struct PackedWord){value};
}

// CHECK-LABEL: define dso_local void @make_packed_word(
// CHECK-SAME: ptr dead_on_unwind noalias writable sret(%struct.PackedWord) align 1 %agg.result,
// CHECK-SAME: i32 noundef signext %value)

imaxdiv_t imaxdiv(long numerator, long denominator) {
  imaxdiv_t result;
  result.quot = numerator / denominator;
  result.rem = numerator % denominator;
  return result;
}

// CHECK-LABEL: define dso_local void @imaxdiv(
// CHECK-SAME: ptr dead_on_unwind noalias writable sret(%struct.imaxdiv_t) align 8 %agg.result,
// CHECK-SAME: i64 noundef %numerator, i64 noundef %denominator)
// CHECK: sdiv i64
// CHECK: store i64 %{{[^, ]+}}, ptr %quot, align 8
// CHECK: srem i64
// CHECK: store i64 %{{[^, ]+}}, ptr %rem{{[0-9]*}}, align 8

imaxdiv_t external_imaxdiv(long numerator, long denominator);

long call_imaxdiv(long numerator, long denominator) {
  imaxdiv_t result = external_imaxdiv(numerator, denominator);
  return result.quot + result.rem;
}

// CHECK-LABEL: define dso_local i64 @call_imaxdiv(
// CHECK: %result = alloca %struct.imaxdiv_t, align 8
// CHECK: call void @external_imaxdiv(
// CHECK-SAME: ptr dead_on_unwind writable sret(%struct.imaxdiv_t) align 8 %result,
// CHECK-SAME: i64 noundef %{{[^,]+}}, i64 noundef %{{[^)]+}})

struct PackedThree external_packed(char value);

struct PackedThree forward_packed(char value) {
  return external_packed(value);
}

// CHECK-LABEL: define dso_local void @forward_packed(
// CHECK-SAME: ptr dead_on_unwind noalias writable sret(%struct.PackedThree) align 1 %agg.result,
// CHECK-SAME: i8 noundef signext %value)
// CHECK: call void @external_packed(
// CHECK-SAME: ptr dead_on_unwind writable sret(%struct.PackedThree) align 1 %agg.result,
// CHECK-SAME: i8 noundef signext %{{[0-9]+}})

typedef struct PackedThree (*packed_factory)(char);

struct PackedThree indirect_packed(packed_factory factory, char value) {
  return factory(value);
}

// CHECK-LABEL: define dso_local void @indirect_packed(
// CHECK-SAME: ptr dead_on_unwind noalias writable sret(%struct.PackedThree) align 1 %agg.result,
// CHECK-SAME: ptr noundef %factory, i8 noundef signext %value)
// CHECK: call void %{{[0-9]+}}(
// CHECK-SAME: ptr dead_on_unwind writable sret(%struct.PackedThree) align 1 %agg.result,
// CHECK-SAME: i8 noundef signext %{{[0-9]+}})

// ISEL-LABEL: name: forward_packed
// ISEL: liveins: $r251, $r231
// ISEL: [[SRET:%[0-9]+]]:{{[^ ]+}} = COPY $r251
// ISEL: $r251 = COPY [[SRET]]
// ISEL: $r231 = COPY
// ISEL: DIRECT_CALL_STATE @external_packed{{.*}}implicit $r251, implicit $r231
// ISEL: $r231 = COPY [[SRET]]
// ISEL: RET_VALUE {{.*}}implicit $r231
// ISEL-LABEL: name: indirect_packed
// ISEL: liveins: $r251, $r231, $r232
// ISEL: [[SRET:%[0-9]+]]:{{[^ ]+}} = COPY $r251
// ISEL: $r251 = COPY [[SRET]]
// ISEL: CALL_STATE {{.*}}implicit $r251, implicit $r231
// ISEL: $r231 = COPY [[SRET]]
// ISEL: RET_VALUE {{.*}}implicit $r231
