// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -std=c17 \
// RUN:   -mrelocation-model static -emit-llvm -disable-llvm-passes -o - %s \
// RUN:   | FileCheck %s
// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -std=c17 -O0 \
// RUN:   -c %s -o %t-O0.o
// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -std=c17 -O2 \
// RUN:   -c %s -o %t-O2.o
// RUN: %clang --target=mmix-unknown-unknown -ffreestanding -std=c17 -Oz \
// RUN:   -c %s -o %t-Oz.o
// RUN: llvm-readobj --file-headers %t-O0.o | FileCheck %s --check-prefix=ELF
// RUN: llvm-readobj --file-headers %t-O2.o | FileCheck %s --check-prefix=ELF
// RUN: llvm-readobj --file-headers %t-Oz.o | FileCheck %s --check-prefix=ELF

enum SignedEnum { SignedEnumValue = -1 };
enum UnsignedEnum { UnsignedEnumValue = 0xffffffffU };

_Bool bool_from_integer(long value) { return value; }
_Bool bool_from_float(double value) { return value; }
_Bool bool_from_pointer(const void *value) { return value; }

// CHECK-LABEL: define dso_local i1 @bool_from_integer(i64 noundef %value)
// CHECK: icmp ne i64 %{{.*}}, 0
// CHECK-LABEL: define dso_local i1 @bool_from_float(double noundef %value)
// CHECK: fcmp une double %{{.*}}, 0.000000e+00
// CHECK-LABEL: define dso_local i1 @bool_from_pointer(ptr noundef %value)
// CHECK: icmp ne ptr %{{.*}}, null

long signed_enum_to_long(enum SignedEnum value) { return value; }
unsigned long unsigned_enum_to_long(enum UnsignedEnum value) { return value; }
long signed_char_to_long(signed char value) { return value; }
unsigned long unsigned_short_to_long(unsigned short value) { return value; }
unsigned short long_to_unsigned_short(long value) { return value; }

// CHECK-LABEL: define dso_local i64 @signed_enum_to_long(i32 noundef signext
// CHECK: sext i32 %{{.*}} to i64
// CHECK-LABEL: define dso_local i64 @unsigned_enum_to_long(i32 noundef zeroext
// CHECK: zext i32 %{{.*}} to i64
// CHECK-LABEL: define dso_local i64 @signed_char_to_long(i8 noundef signext
// CHECK: sext i8 %{{.*}} to i64
// CHECK-LABEL: define dso_local i64 @unsigned_short_to_long(i16 noundef zeroext
// CHECK: zext i16 %{{.*}} to i64
// CHECK-LABEL: define dso_local i16 @long_to_unsigned_short(i64 noundef %value)
// CHECK: trunc i64 %{{.*}} to i16

unsigned long pointer_to_integer(const void *value) {
  return (unsigned long)value;
}

void *integer_to_pointer(unsigned long value) { return (void *)value; }

// CHECK-LABEL: define dso_local i64 @pointer_to_integer(ptr noundef %value)
// CHECK: ptrtoint ptr %{{.*}} to i64
// CHECK-LABEL: define dso_local ptr @integer_to_pointer(i64 noundef %value)
// CHECK: inttoptr i64 %{{.*}} to ptr

float signed_to_float(long value) { return (float)value; }
double unsigned_to_double(unsigned long value) { return (double)value; }
int float_to_signed(float value) { return (int)value; }
unsigned long double_to_unsigned(double value) {
  return (unsigned long)value;
}
float long_double_to_float(long double value) { return (float)value; }

// CHECK-LABEL: define dso_local float @signed_to_float(i64 noundef %value)
// CHECK: sitofp i64 %{{.*}} to float
// CHECK-LABEL: define dso_local double @unsigned_to_double(i64 noundef %value)
// CHECK: uitofp i64 %{{.*}} to double
// CHECK-LABEL: define dso_local i32 @float_to_signed(float noundef %value)
// CHECK: fptosi float %{{.*}} to i32
// CHECK-LABEL: define dso_local i64 @double_to_unsigned(double noundef %value)
// CHECK: fptoui double %{{.*}} to i64
// CHECK-LABEL: define dso_local float @long_double_to_float(double noundef %value)
// CHECK: fptrunc double %{{.*}} to float

// ELF: Format: elf64-mmix
// ELF: Type: Relocatable
// ELF: Machine: EM_MMIX
