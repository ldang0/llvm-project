// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c17 \
// RUN:   -mrelocation-model static -emit-llvm -disable-llvm-passes -o - %s \
// RUN:   | FileCheck %s
// RUN: %clang --target=mmix-unknown-elf -ffreestanding -std=c17 \
// RUN:   -S -emit-llvm -fno-discard-value-names \
// RUN:   -Xclang -disable-llvm-passes -o - %s \
// RUN:   | FileCheck %s
// RUN: %clang --target=mmix-unknown-elf -ffreestanding -std=c17 -O1 \
// RUN:   -c -o /dev/null %s

typedef _Complex float complex_float;
typedef _Complex double complex_double;
typedef _Complex long double complex_long_double;

// CHECK: target triple = "mmix-unknown-unknown"

complex_float identity_float(complex_float value) { return value; }
complex_double identity_double(complex_double value) { return value; }
complex_long_double identity_long_double(complex_long_double value) {
  return value;
}

// CHECK-LABEL: define dso_local i64 @identity_float(i64 noundef %value.coerce)
// CHECK: ret i64
// CHECK-LABEL: define dso_local { double, double } @identity_double(
// CHECK-SAME: ptr noundef byval({ double, double }) align 8 %value)
// CHECK: ret { double, double }
// CHECK-LABEL: define dso_local { double, double } @identity_long_double(
// CHECK-SAME: ptr noundef byval({ double, double }) align 8 %value)
// CHECK: ret { double, double }

extern complex_float consume_float(complex_float);
extern complex_double consume_double(complex_double);
extern complex_long_double consume_long_double(complex_long_double);

complex_float call_float(complex_float value) {
  return consume_float(value);
}

complex_double call_double(complex_double value) {
  return consume_double(value);
}

complex_long_double call_long_double(complex_long_double value) {
  return consume_long_double(value);
}

// CHECK-LABEL: define dso_local i64 @call_float(i64 noundef %value.coerce)
// CHECK: call i64 @consume_float(i64 noundef
// CHECK-LABEL: define dso_local { double, double } @call_double(
// CHECK-SAME: ptr noundef byval({ double, double }) align 8 %value)
// CHECK: call { double, double } @consume_double(
// CHECK-SAME: ptr noundef byval({ double, double }) align 8
// CHECK-LABEL: define dso_local { double, double } @call_long_double(
// CHECK-SAME: ptr noundef byval({ double, double }) align 8 %value)
// CHECK: call { double, double } @consume_long_double(
// CHECK-SAME: ptr noundef byval({ double, double }) align 8

typedef complex_double (*complex_callback)(complex_double);

complex_double call_callback(complex_callback callback, complex_double value) {
  return callback(value);
}

// CHECK-LABEL: define dso_local { double, double } @call_callback(
// CHECK-SAME: ptr noundef %callback,
// CHECK-SAME: ptr noundef byval({ double, double }) align 8 %value)
// CHECK: call { double, double } %{{[^ (]+}}(
// CHECK-SAME: ptr noundef byval({ double, double }) align 8
