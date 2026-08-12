// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -mrelocation-model static \
// RUN:   -emit-llvm -disable-llvm-passes -o - %s | FileCheck %s

typedef unsigned short (*ScalarFunction)(signed char, unsigned int, double,
                                         long long, unsigned long long, void *);

unsigned short call_indirect(ScalarFunction fn, signed char c, unsigned int u,
                             double d, long long ll, unsigned long long ull,
                             void *p) {
  return fn(c, u, d, ll, ull, p);
}

// CHECK-LABEL: define dso_local i16 @call_indirect(
// CHECK-SAME: ptr noundef %fn, i8 noundef signext %c,
// CHECK-SAME: i32 noundef zeroext %u, double noundef %d, i64 noundef %ll,
// CHECK-SAME: i64 noundef %ull, ptr noundef %p)
// CHECK: %call = call i16 %{{[0-9]+}}(i8 noundef signext %{{[0-9]+}},
// CHECK-SAME: i32 noundef zeroext %{{[0-9]+}},
// CHECK-SAME: double noundef %{{[0-9]+}}, i64 noundef %{{[0-9]+}},
// CHECK-SAME: i64 noundef %{{[0-9]+}}, ptr noundef %{{[0-9]+}})
// CHECK: ret i16 %call

char return_narrow(char value) { return value; }

int extend_narrow_result(void) { return return_narrow(1); }

// CHECK-LABEL: define dso_local i8 @return_narrow(i8 noundef signext
// CHECK-LABEL: define dso_local i32 @extend_narrow_result()
// CHECK: %call = call i8 @return_narrow(i8 noundef signext 1)
// CHECK: %conv = sext i8 %call to i32
// CHECK: ret i32 %conv

void consume_promoted(int marker, ...);

void call_default_promotions(char c, unsigned char uc, float f) {
  consume_promoted(0, c, uc, f);
}

// CHECK-LABEL: define dso_local void @call_default_promotions(
// CHECK-SAME: i8 noundef signext %c, i8 noundef zeroext %uc,
// CHECK-SAME: float noundef %f)
// CHECK: call void (i32, ...) @consume_promoted(i32 noundef signext 0,
// CHECK-SAME: i32 noundef signext %{{[a-z0-9]+}},
// CHECK-SAME: i32 noundef signext %{{[a-z0-9]+}},
// CHECK-SAME: double noundef %{{[a-z0-9]+}})

// CHECK: declare dso_local void @consume_promoted(i32 noundef signext, ...)
