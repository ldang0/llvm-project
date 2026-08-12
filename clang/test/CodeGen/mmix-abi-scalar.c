// REQUIRES: mmix-registered-target
// RUN: %clang_cc1 -triple mmix-unknown-unknown -mrelocation-model static \
// RUN:   -emit-llvm -disable-llvm-passes -o - %s | FileCheck %s

typedef enum SignedEnum { SignedValue = -1 } SignedEnum;
typedef enum UnsignedEnum { UnsignedValue = 0xffffffffU } UnsignedEnum;

void consume_scalars(_Bool b, char c, signed char sc, unsigned char uc,
                     short s, unsigned short us, int i, unsigned int ui,
                     SignedEnum se, UnsignedEnum ue, long l,
                     unsigned long ul, void *p, void (*fp)(void), float f,
                     double d, long double ld);

_Bool return_bool(void) { return 1; }
char return_char(char value) { return value; }
unsigned char return_uchar(unsigned char value) { return value; }
short return_short(short value) { return value; }
unsigned short return_ushort(unsigned short value) { return value; }
int return_int(int value) { return value; }
unsigned int return_uint(unsigned int value) { return value; }
long return_long(long value) { return value; }
void *return_pointer(void *value) { return value; }
float return_float(float value) { return value; }
double return_double(double value) { return value; }
long double return_long_double(long double value) { return value; }

// CHECK-LABEL: define dso_local i1 @return_bool()
// CHECK-LABEL: define dso_local i8 @return_char(i8 noundef signext
// CHECK-LABEL: define dso_local i8 @return_uchar(i8 noundef zeroext
// CHECK-LABEL: define dso_local i16 @return_short(i16 noundef signext
// CHECK-LABEL: define dso_local i16 @return_ushort(i16 noundef zeroext
// CHECK-LABEL: define dso_local i32 @return_int(i32 noundef signext
// CHECK-LABEL: define dso_local i32 @return_uint(i32 noundef zeroext
// CHECK-LABEL: define dso_local i64 @return_long(i64 noundef
// CHECK-LABEL: define dso_local ptr @return_pointer(ptr noundef
// CHECK-LABEL: define dso_local float @return_float(float noundef
// CHECK-LABEL: define dso_local double @return_double(double noundef
// CHECK-LABEL: define dso_local double @return_long_double(double noundef

void call_scalars(void) {
  consume_scalars(1, -2, -3, 4, -5, 6, -7, 8, SignedValue, UnsignedValue, 9,
                  10, (void *)0, (void (*)(void))0, 1.0f, 2.0, 3.0L);
}

// CHECK-LABEL: define{{.*}} void @call_scalars()
// CHECK: call void @consume_scalars(i1 noundef zeroext true,
// CHECK-SAME: i8 noundef signext -2, i8 noundef signext -3,
// CHECK-SAME: i8 noundef zeroext 4, i16 noundef signext -5,
// CHECK-SAME: i16 noundef zeroext 6, i32 noundef signext -7,
// CHECK-SAME: i32 noundef zeroext 8, i32 noundef signext -1,
// CHECK-SAME: i32 noundef zeroext -1, i64 noundef 9, i64 noundef 10,
// CHECK-SAME: ptr noundef null, ptr noundef null, float noundef 1.000000e+00,
// CHECK-SAME: double noundef 2.000000e+00, double noundef 3.000000e+00)

// CHECK: declare dso_local void @consume_scalars(i1 noundef zeroext,
// CHECK-SAME: i8 noundef signext, i8 noundef signext, i8 noundef zeroext,
// CHECK-SAME: i16 noundef signext, i16 noundef zeroext, i32 noundef signext,
// CHECK-SAME: i32 noundef zeroext, i32 noundef signext, i32 noundef zeroext,
// CHECK-SAME: i64 noundef, i64 noundef, ptr noundef, ptr noundef,
// CHECK-SAME: float noundef, double noundef, double noundef)
